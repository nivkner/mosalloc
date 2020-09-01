#include <iostream>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#include "gtest/gtest.h"
#include "MemoryIntervalList.h"

TEST(MemoryIntervalListTest, MixedIntervals) {
    MemoryIntervalList l;
    l.Initialize(mmap, munmap, 10);
    l.AddInterval(1<<20, 1<<30, PageSize::HUGE_2MB);
    l.AddInterval(1<<10, 1<<20, PageSize::BASE_4KB);
    l.AddInterval(0, 1<<10, PageSize::HUGE_1GB);

    EXPECT_EQ(l.At(0)._start_offset, (1<<20));
    EXPECT_EQ(l.At(1)._start_offset, (1<<10));
    EXPECT_EQ(l.At(2)._start_offset, (0));

    l.Sort();

    EXPECT_EQ(l.At(2)._start_offset, (1<<20));
    EXPECT_EQ(l.At(1)._start_offset, (1<<10));
    EXPECT_EQ(l.At(0)._start_offset, (0));
}
