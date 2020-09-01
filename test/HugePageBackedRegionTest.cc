//
// Created by a.mohammad on 3/10/2019.
//
// Note: hugepages should be pre-allocated before running this test (at least 2 pages of 1GB and 1024 pages of 2MB
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <cstdlib>

#include <string>
#include <iostream>
#include <cstdio>
#include <array>

#include "HugePageBackedRegion.h"
#include "NumaMaps.h"
#include "globals.h"
#include "gtest/gtest.h"

#define GB (1073741824)
#define MB (1048576)

class HugePageBackedRegionTest : public ::testing::Test {
 public:
  HugePageBackedRegion _hpbr;
  HugePageBackedRegionTest() : _hpbr() {}
  ~HugePageBackedRegionTest() {}

  void SetUp() override {
    // reserve huge pages as the max region size of HugePageBackedRegion tests
    // currently it's set to 2GB, in case further tests will be added with
    // larger regions, then the reserve command should be adapted accordingly
    std::string cmd = "reserveHugePages.sh -n0 -l1024 -h2";
    std::string cwd(get_current_dir_name());
    cmd = cwd + "/" + cmd;
    ASSERT_EQ(system(cmd.c_str()), 0);
  }

  void test_numa_maps(void *start_addr, size_t size, size_t page_size) {
    NumaMaps numa_maps(getpid());
    EXPECT_EQ(numa_maps.GetMemoryRange(start_addr)._total_size, size);
    auto memory_range = numa_maps.GetMemoryRange(start_addr);
    EXPECT_EQ(memory_range._total_size, size);
    EXPECT_EQ(static_cast<size_t>(memory_range._page_size), page_size);
    EXPECT_EQ(memory_range._type, NumaMaps::MemoryRange::Type::ANONYMOUS);
    EXPECT_EQ(memory_range._total_pages, (size / page_size));
  }

  void test_huge_page_backed_region(size_t size,
                                    off_t start_1gb,
                                    off_t end_1gb,
                                    off_t start_2mb,
                                    off_t end_2mb) {
    _hpbr.Initialize(size, start_1gb, end_1gb, start_2mb, end_2mb, mmap, munmap);
    void *region_base = _hpbr.GetRegionBase();
    _hpbr.Resize(0);
    _hpbr.Resize(size);
    memset(region_base, -1, size);

    off_t start_first_huge_page = start_1gb;
    off_t end_first_huge_page = end_1gb;
    off_t start_second_huge_page = start_2mb;
    off_t end_second_huge_page = end_2mb;
    size_t page_size_first = static_cast<size_t>(PageSize::HUGE_1GB);
    size_t page_size_second = static_cast<size_t>(PageSize::HUGE_2MB);
    if (start_2mb < start_first_huge_page) {
      start_first_huge_page = start_2mb;
      end_first_huge_page = end_2mb;
      start_second_huge_page = start_1gb;
      end_second_huge_page = end_1gb;
      page_size_first = static_cast<size_t>(PageSize::HUGE_2MB);
      page_size_second = static_cast<size_t>(PageSize::HUGE_1GB);
    }

    off_t start_current = 0;
    if ((end_first_huge_page - start_first_huge_page) > 0) {
      if (start_current < start_first_huge_page) {
        test_numa_maps((void *) ((off_t) region_base + start_current),
                       (size_t) start_first_huge_page,
                       static_cast<size_t>(PageSize::BASE_4KB));
        start_current = start_first_huge_page;
      }
      test_numa_maps((void *) ((off_t) region_base + start_current),
                     (size_t) (end_first_huge_page - start_first_huge_page),
                     page_size_first);
      start_current = end_first_huge_page;
    }
    if ((end_second_huge_page - start_second_huge_page) > 0) {
      if (start_current < start_second_huge_page) {
        test_numa_maps((void *) ((off_t) region_base + start_current),
                       (size_t) (start_second_huge_page - start_current),
                       static_cast<size_t>(PageSize::BASE_4KB));
        start_current = start_second_huge_page;
      }
      test_numa_maps((void *) ((off_t) region_base + start_current),
                     (size_t) (end_second_huge_page - start_second_huge_page),
                     page_size_second);
      start_current = end_second_huge_page;
    }
    if (start_current < (off_t) size) {
      test_numa_maps((void *) ((off_t) region_base + start_current),
                     (size_t) (size - start_current),
                     static_cast<size_t>(PageSize::BASE_4KB));
    }
    _hpbr.Resize(0);
  }
};

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_1GB) {
  test_huge_page_backed_region(1 * GB, 0, 1 * GB, 0, 0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_1GB_2MB) {
  test_huge_page_backed_region(2ULL * GB, 0, 1 * GB, 1 * GB, 2ULL * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_2MB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 0, 1 * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_2MB_1GB) {
  test_huge_page_backed_region(2ULL * GB, 1 * GB, 2ULL * GB, 0, 1 * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 0, 0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_1GB) {
  test_huge_page_backed_region(2ULL * GB, 1 * GB, 2ULL * GB, 0, 0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_1GB_4KB) {
  test_huge_page_backed_region(2ULL * GB, 100 * MB, (100 * MB + 1 * GB), 0,0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_1GB_4KB_2MB_4KB) {
  size_t size = 2ULL * GB;
  off_t start_1gb = 100 * MB;
  off_t end_1gb = start_1gb + 1 * GB;
  off_t start_2mb = end_1gb + 100 * MB;
  off_t end_2mb = start_2mb + 100 * MB;
  test_huge_page_backed_region(size, start_1gb, end_1gb, start_2mb, end_2mb);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 100 * MB, 1 * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB_4KB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 101 * MB, 201 * MB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB_4KB_1GB_4KB) {
  size_t size = 2ULL * GB;
  off_t start_2mb = 11 * MB;
  off_t end_2mb = start_2mb + 100 * MB;
  off_t start_1gb = end_2mb + 100 * MB;
  off_t end_1gb = start_1gb + 1 * GB;

  test_huge_page_backed_region(size, start_1gb, end_1gb, start_2mb, end_2mb);
}

#define WRITTEN_DATA (0x34)
using namespace std;
void ValidateData(char *ptr, size_t size) {
  for (unsigned int i=0; i<size; i += (MB)) {
    ASSERT_EQ(ptr[i], WRITTEN_DATA)
        << "Written data was overwritten: written-data: " << WRITTEN_DATA
        << " , new-data: " << ptr[i];
  }
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB_Unaligned) {
  size_t size = 10*MB;
  size_t start_2mb = 3*MB;
  size_t end_2mb = 5*MB;
  _hpbr.Initialize(size, 0, 0, start_2mb, end_2mb, mmap, munmap);
  void *region_base = _hpbr.GetRegionBase();
  
  for (unsigned int i=0; i<=size; i += (MB)) {
    _hpbr.Resize(i);
    memset(region_base, WRITTEN_DATA, i);
    ValidateData((char*)region_base, i);
  }
  
  for (unsigned int i=0; i<=size; i += (MB)) {
    _hpbr.Resize(size-i);
    ValidateData((char*)region_base, size-i);
  }
  _hpbr.Resize(0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_1GB_AndResize) {
  size_t size = 1*GB;
  _hpbr.Initialize(size, 0, size, 0, 0, mmap, munmap);
  void *region_base = _hpbr.GetRegionBase();
  _hpbr.Resize(0);
  _hpbr.Resize(size);
  memset(region_base, WRITTEN_DATA, size);

  ValidateData((char*)region_base, size);

  for (unsigned int i=size; i>0; i -= (2*MB)) {
    _hpbr.Resize(i);
    ValidateData((char*)region_base, size);
  }
  _hpbr.Resize(0);
}
