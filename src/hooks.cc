#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <execinfo.h>

#include "hooks.h"
#include "GlibcAllocationFunctions.h"
#include "MemoryAllocator.h"

static void constructor() __attribute__((constructor));
static void destructor() __attribute__((destructor));
static void setup_morecore();

MemoryAllocator hpbrs_allocator;
void* sys_heap_top = nullptr;
void* brk_top = nullptr;
bool is_library_initialized = false;
std::mutex g_hook_sbrk_mutex;
std::mutex g_hook_brk_mutex;
std::mutex g_hook_mmap_mutex;
//std::mutex g_hook_malloc_mutex;

//void *(*__morecore)(ptrdiff_t) = sbrk;
//__morecore = sbrk;

/**
 * this flag is used to prevent potential deadlock in some memory allocators 
 * (such as dlmalloc):
 * is_inside_malloc_api will be toggled to true if and only if we are inside
 * some malloc API which is intercepted in this file (i.e., either malloc, 
 * calloc, or free).
 * This flag is used to prevent deadlocks caused by a recursive calls to these 
 * malloc APIs.
 * The recursive calls could be happened (as our observations) only in case
 * that dlsym calls _dlerror_run before their local buffers were initialized
 * and then _dlerror_run will call calloc to allocate the error buffer.
 * We recorded few scenarios where this could happen:
 * 1) malloc* (during loading time) -> dlsym (to call glibc-malloc) -> 
 *    _dlerror_run -> calloc*
 * 2) free* (after we already loaded and initialized) -> deallocate memory from
 *    internal pools -> resize pool (shrink) -> dlsym (to call glibc-munmap)
 *    -> _dlerror_run -> calloc*
 * APIs marked with (*) are the APIs that acquires the malloc API lock.
 *
**/
bool is_inside_malloc_api = false;

// the file to write the mallocs+backtrace to
static int mosalloc_log = -1;
// the file containing the data on which allocation should be made huge page
static int huge_allocs = -1;

// write exactly len bytes from input to output_fd
// return result of final write
static int write_all(int output_fd, char *input, int len) {
	int bytes_written = 0;
	int res = 0;
	while ((res = write(output_fd, &input[bytes_written], len - bytes_written)) > 0) {
		bytes_written += res;
	}
	return res;
}

static void setup_morecore() {
    GlibcAllocationFunctions local_glibc_funcs;
    void* temp_brk_top = local_glibc_funcs.CallGlibcSbrk(0);
    temp_brk_top = (void*)ROUND_UP((size_t)temp_brk_top, PageSize::HUGE_1GB);
    _brk_region_base = temp_brk_top;
    
    // disable mmap by setting M_MMAP_MAX to 0 in order to force glibc to
    // allocate all memory chunks through morecore (to prevent using internal
    // glibc mmap for allocating memory chunks greater than M_MMAP_THRESHOLD
    mallopt(M_MMAP_MAX, 0);

    // disable heap trimming to prevent shrinking the heap below our heap base
    // address (which is an aligned---rounded up---address of the system heap
    // base to the nearest hugepage aligned address. This is done because we
    // are not allowed to violate brk semantics and to prevent heap corruption.
    mallopt(M_TRIM_THRESHOLD, -1);

    // Limit glibc malloc arenas to 1 arena. This is done to prevent glibc from
    // creating new arenas by calling its internal mmap (and then we cannot
    // intercept it and back it with hugepages). Glibc creates  additional
    // memory allocation arenas if mutex contention is detected (in a
    // multi-threaded applications)
    mallopt(M_ARENA_MAX, 1);

    char *huge_allocs_path = getenv("MOSALLOC_HUGE");

    if (huge_allocs_path) {
	    huge_allocs = open("mosalloc.log", O_RDONLY);
    } else {
	    mosalloc_log = open("mosalloc.log", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    }

    __morecore = mosalloc_morecore;
}

static void activate_mosalloc() {
    is_library_initialized = true;
    setup_morecore();
}

static void deactivate_mosalloc() {
    hpbrs_allocator.AnalyzeRegions();
    is_library_initialized = false;
}

extern char *__progname;
static void constructor() {
    if (!strcmp(__progname, "orted")) {
        return;
    }
    //printf("\n%s\n", __progname);
    activate_mosalloc();
}

static void destructor() {
    deactivate_mosalloc();
}

int mprotect(void *addr, size_t len, int prot) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == true &&
        hpbrs_allocator.IsAddressInHugePageRegions(addr) == true) {
        return 0;
    }
    GlibcAllocationFunctions local_glibc_funcs;
    return local_glibc_funcs.CallGlibcMprotect(addr, len, prot);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, 
            off_t offset) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcMmap(addr, length, prot, flags, fd, offset);
    }
    
    MUTEX_GUARD(g_hook_mmap_mutex);

    if (fd >= 0) {
        return hpbrs_allocator.AllocateFromFileMmapRegion(addr, length, prot, flags, fd, offset);
        //GlibcAllocationFunctions local_glibc_funcs;
        //return local_glibc_funcs.CallGlibcMmap(addr, length, prot, flags, fd, offset);
    }
    return hpbrs_allocator.AllocateFromAnonymousMmapRegion(length);
}

int munmap(void *addr, size_t length) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcMunmap(addr, length);
    }

    MUTEX_GUARD(g_hook_mmap_mutex);

    int res = hpbrs_allocator.DeallocateFromMmapRegion(addr, length);
    return res;
}

int brk(void *addr) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcBrk(addr);
    }
    
    MUTEX_GUARD(g_hook_brk_mutex);

    return hpbrs_allocator.ChangeProgramBreak(addr);
}

void *mosalloc_morecore(intptr_t increment) __THROW_EXCEPTION {
    return sbrk(increment);
}

void *sbrk(intptr_t increment) __THROW_EXCEPTION {
    char text[100];

    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcSbrk(increment);
    }
    
    MUTEX_GUARD(g_hook_sbrk_mutex);

    // if this the first call to sbrk after pools were initialized
    // then initialize brk_top to be the brk pool base address
    if (brk_top == nullptr) {
        brk_top = hpbrs_allocator.GetBrkRegionBase();
    }

    /*
     * On success, sbrk() returns the previous program break.  (If the break 
     * was increased, then this value is a pointer to the start of the newly 
     * allocated memory).  On error, (void *) -1 is returned, and errno is 
     * set to ENOMEM.
    */
    
    void* prev_brk = brk_top;

    if (huge_allocs < 0) {
	    if (increment < 0) {
		    // when gathering data, refuse to decrement the heap,
		    // to ensure every allocation occurse in a seperate location
		    return prev_brk;
	    }
    }

    void* new_brk = (void*) ((intptr_t)brk_top + increment);

    if (brk(new_brk) < 0) {
        return ((void*)-1);
    }

    void *addresses[32];
    int trace_size = backtrace(addresses, 32);
    if (huge_allocs < 0) {
	    // use sprintf to avoid allocation inside morecore
	    int len = sprintf(text, "\n%p\n%p\n", prev_brk, new_brk);
	    write_all(mosalloc_log, text, len);
	    backtrace_symbols_fd(addresses, trace_size, mosalloc_log);
    } else {
	    // TODO allocate with huge pages based on data
    }

    brk_top = new_brk;
    return prev_brk;
}


