#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/syscall.h>
#include <assert.h>
#include "MemoryAllocator.h"

/*
#ifdef THREAD_SAFETY
#define MUTEX_GUARD(lock) std::lock_guard<std::mutex> guard(lock) 
#else //THREAD_SAFETY
#define MUTEX_GUARD(lock)
#endif //THREAD_SAFETY
*/

#define RESIZE_THRESHOLD (2097152)

void *_brk_region_base = 0;

void* GlibcMmap(void *addr, size_t length, int prot, int flags,
                int fd, off_t offset) {
    static GlibcAllocationFunctions glibc_funcs;
    return glibc_funcs.CallGlibcMmap(addr, length, prot, flags, fd, offset);
}

int GlibcMunmap(void *addr, size_t length) {
    static GlibcAllocationFunctions glibc_funcs;
    return glibc_funcs.CallGlibcMunmap(addr, length);
}


void MemoryAllocator::InitRegions(void *brk_region_base) {
    HugePagesConfiguration hppc;
    auto mmap_params = hppc.ReadFromEnvironmentVariables
            (HugePagesConfiguration::ConfigType::MMAP_POOL);
    _mmap_anon_hpbr.Initialize(mmap_params._pool_size,
                               mmap_params._1gb_start,
                               mmap_params._1gb_end,
                               mmap_params._2mb_start,
                               mmap_params._2mb_end,
                               GlibcMmap,
                               GlibcMunmap);

    void* start = _mmap_anon_hpbr.GetRegionBase();
    void* end = (void*)((size_t)start + mmap_params._pool_size);
    _mmap_anon_ffa.Initialize(mmap_params._ffa_list_size, start, end, GlibcMmap,
                              GlibcMunmap);

    auto mmap_file_params = hppc.ReadFromEnvironmentVariables
            (HugePagesConfiguration::ConfigType::FILE_BACKED_POOL);
    _mmap_file_hpbr.Initialize(mmap_file_params._pool_size,
                               0, 0, 0, 0,
                               GlibcMmap,
                               GlibcMunmap);

    void* mmap_file_start = _mmap_file_hpbr.GetRegionBase();
    void* mmap_file_end = (void*)((size_t)start + mmap_file_params._pool_size);
    _mmap_file_ffa.Initialize(mmap_file_params._ffa_list_size, mmap_file_start,
                              mmap_file_end, GlibcMmap, GlibcMunmap);

    auto brk_params = hppc.ReadFromEnvironmentVariables
            (HugePagesConfiguration::ConfigType::BRK_POOL);
    _brk_hpbr.Initialize(brk_params._pool_size,
                         brk_params._1gb_start,
                         brk_params._1gb_end,
                         brk_params._2mb_start,
                         brk_params._2mb_end,
                         GlibcMmap,
                         GlibcMunmap,
                         brk_region_base);

    _mmap_anon_hpbr.Resize(0);
    _mmap_file_hpbr.Resize(0);
    _brk_hpbr.Resize(0);

    _anon_mmap_max_size = 0;
    _file_mmap_max_size = 0;
    _brk_max_size = 0;

    auto general_params = hppc.GetGeneralParams();
    _analyze_hpbrs = general_params._analyze_hpbrs;

    if (_analyze_hpbrs) {
        void* anon_start = _mmap_anon_hpbr.GetRegionBase();
        void* anon_end = PTR_ADD(anon_start, _mmap_anon_hpbr.GetRegionMaxSize());
        void* brk_start = _brk_hpbr.GetRegionBase();
        void* brk_end = PTR_ADD(brk_start, _brk_hpbr.GetRegionMaxSize());
        void* file_start = _mmap_file_hpbr.GetRegionBase();
        void* file_end = PTR_ADD(file_start, _mmap_file_hpbr.GetRegionMaxSize());

        pid_t pid = getpid();
        pid_t tid = syscall(SYS_gettid);
        FILE *log_file = fopen ("pools_base_pointers.out", "a+");
        // check if the file is empty
        if (!fseek(log_file, 0, SEEK_END)) {
            unsigned long len = (unsigned long)ftell(log_file);
            if (len == 0) {
                fprintf(log_file, 
                        "pid,tid,anon-mmap-start,anon-mmap-end,brk-start,brk-end,file-mmap-start,file-mmap-end\n");
            }
        }
        fprintf(log_file, "%d,%d,%p,%p,%p,%p,%p,%p\n",
                pid, tid,
                anon_start, anon_end,
                brk_start, brk_end,
                file_start, file_end);
        fclose(log_file);
    }
        
}

MemoryAllocator::MemoryAllocator() : 
    _isInitialized(true), _analyze_hpbrs(false),
    _anon_mmap_max_size(0), _file_mmap_max_size(0), _brk_max_size(0)
{
    InitRegions(_brk_region_base);
}

MemoryAllocator::~MemoryAllocator() {
    _isInitialized = false;
}

void MemoryAllocator::AnalyzeRegions() {

    if (_analyze_hpbrs) {
        /* Write regions max size to log file */
        std::string pid_str;
        std::stringstream out;
        out << getpid();
        pid_str = out.str();

        std::string fileName = "mosalloc_hpbrs_sizes." + pid_str + ".csv";
        FILE *log_file = fopen (fileName.c_str(), "w+");
        fprintf(log_file, "region,max-size\n");
        fprintf(log_file, "brk,%lu\n", _brk_max_size);
        fprintf(log_file, "anon-mmap,%lu\n", _anon_mmap_max_size);
        fprintf(log_file, "file-mmap,%lu\n", _file_mmap_max_size);
        fclose(log_file);
        /*
           std::string fileName = "mosalloc_hpbrs_sizes." + pid_str + ".csv";
           FILE *log_file = fopen (fileName.c_str(), "w+");
           fprintf(log_file, "brk,anon-mmap,file-mmap\n");
           fprintf(log_file, "%lu,%lu,%lu\n", _brk_max_size, _anon_mmap_max_size, 
           _file_mmap_max_size);
           fclose(log_file);
           */
    }
}

void* MemoryAllocator::GetBrkRegionBase() {
    return _brk_hpbr.GetRegionBase();
}

void* MemoryAllocator::AllocateFromAnonymousMmapRegion(size_t length) {
    MUTEX_GUARD(_anon_mmap_mutex);

    void *ptr = _mmap_anon_ffa.Allocate(length);
    if (ptr == NULL) {
        THROW_EXCEPTION("Anonymous mmap pool is out of memory\n");
    }
    size_t hpbr_top_addr = (size_t)_mmap_anon_hpbr.GetRegionBase() +
            _mmap_anon_hpbr.GetRegionSize();
    size_t alloc_mem_top_addr = (size_t)ptr + length;

    if (alloc_mem_top_addr > hpbr_top_addr) {
        size_t size = alloc_mem_top_addr - (size_t)_mmap_anon_hpbr.GetRegionBase();
        _mmap_anon_hpbr.Resize(size);
    }

    if (_anon_mmap_max_size < _mmap_anon_hpbr.GetRegionSize()) {
        _anon_mmap_max_size = _mmap_anon_hpbr.GetRegionSize();
    }

    return ptr;
}

void* MemoryAllocator::AllocateFromFileMmapRegion(
        void *addr, size_t length, int prot, 
        int flags, int fd, off_t offset) {
    MUTEX_GUARD(_file_mmap_mutex);

    void* ptr = addr;
    if (ptr == NULL) {
        ptr = _mmap_file_ffa.Allocate(length);
        if (ptr == NULL) {
            THROW_EXCEPTION("File mmap pool is out of memory\n");
        }
    }

    size_t ffa_max_size = (size_t)_mmap_file_ffa.GetTopAddress() - (size_t)_mmap_file_hpbr.GetRegionBase();
    if (_file_mmap_max_size < ffa_max_size) {
        _file_mmap_max_size = ffa_max_size;
    }

    return GlibcMmap(ptr, length, prot, MAP_FIXED | flags, fd, offset);
}

int MemoryAllocator::DeallocateFromAnonymousMmapRegion(void* addr, size_t length) {
    MUTEX_GUARD(_anon_mmap_mutex);
    int res = _mmap_anon_ffa.Free(addr, length);
    size_t ffa_top_size = (size_t)(PTR_SUB(_mmap_anon_ffa.GetTopAddress(),
                                           _mmap_anon_hpbr.GetRegionBase()));
    if (res == 0
        && ffa_top_size < _mmap_anon_hpbr.GetRegionSize()) {
        if ((_mmap_anon_hpbr.GetRegionSize() - ffa_top_size)
            > RESIZE_THRESHOLD) {
            return _mmap_anon_hpbr.Resize(ffa_top_size);
        }
    }

    return res;
}

int MemoryAllocator::DeallocateFromFileMmapRegion(void* addr, size_t length) {
    MUTEX_GUARD(_file_mmap_mutex);
    int res = _mmap_file_ffa.Free(addr, length);
    if (res < 0) {
        return res;
    }
    return GlibcMunmap(addr, length);
}

int MemoryAllocator::ChangeProgramBreak(void *addr) {
    MUTEX_GUARD(_brk_mutex);

    /* 
     * On success, brk() returns zero.  On error, -1 is returned, 
     * and errno is set to ENOMEM. 
    */
    size_t new_size = (size_t)addr - (size_t)_brk_hpbr.GetRegionBase();
    if (addr < _brk_hpbr.GetRegionBase() ||
        _brk_hpbr.Resize(new_size) != 0) {
        errno = ENOMEM;
        
        return -1;
    }

    if (_brk_max_size < _brk_hpbr.GetRegionSize()) {
        _brk_max_size = _brk_hpbr.GetRegionSize();
    }

    return 0;
}

int MemoryAllocator::DeallocateFromMmapRegion(void *addr, size_t size) {
    _anon_mmap_mutex.lock();
    bool isAddrInAnonMmapPool = _mmap_anon_ffa.Contains(addr);
    _anon_mmap_mutex.unlock();

    _file_mmap_mutex.lock();
    bool isAddrInFileMmapPool = _mmap_file_ffa.Contains(addr);
    _file_mmap_mutex.unlock();

    if (isAddrInAnonMmapPool) {
        return DeallocateFromAnonymousMmapRegion(addr, size);
    }
    else if (isAddrInFileMmapPool) {
        return DeallocateFromFileMmapRegion(addr, size);
    }
    else {
        return -1;
    }
}

bool MemoryAllocator::IsAddressInHugePageRegions(void *addr) {
    if (_isInitialized == false)
        return false;

    bool isAddrInAnonMmapPool = _mmap_anon_ffa.Contains(addr);

    bool isAddrInFileMmapPool = _mmap_file_ffa.Contains(addr);
    
    bool isAddrInBrkPool = (addr >= _brk_hpbr.GetRegionBase() &&
                            addr < PTR_ADD(_brk_hpbr.GetRegionBase(),
                                            _brk_hpbr.GetRegionMaxSize()));

    return (isAddrInAnonMmapPool || isAddrInFileMmapPool || isAddrInBrkPool);
}
