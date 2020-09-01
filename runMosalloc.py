#! /usr/bin/env python3

import sys
import os

class MemoryRegion:
    KB2B = 1024
    MB2B = 1024*KB2B
    GB2B = 1024*MB2B
    BASE_PAGE_SIZE = 4 * KB2B
    LARGE_PAGE_SIZE = 2 * MB2B
    HUGE_PAGE_SIZE = GB2B


    def convertSizeStringToBytes(size_string):
        if size_string.isdigit():
            size = int(size_string)
        else: # the string should contain units
            string_length = len(size_string)
            number = int(size_string[0 : string_length-2])
            units_string = size_string[string_length-2 : string_length].lower()
            if (units_string == "gb"):
                units = MemoryRegion.GB2B
            elif (units_string == "mb"):
                units = MemoryRegion.MB2B
            elif (units_string == "kb"):
                units = MemoryRegion.KB2B
            else:
                sys.exit("Error: invalid string for the size")
            size = number * units
        return size

    def isAligned(num, alignment):
        return ((num % alignment) == 0)

    def validateRegionOffsets(start_offset, end_offset, page_size):
        region_size = end_offset - start_offset
        # validate that end >= start
        if (region_size < 0):
            sys.exit("Error: end_offset < start_offset")
        # validate that the region is aligned
        if ((not MemoryRegion.isAligned(start_offset, MemoryRegion.BASE_PAGE_SIZE)) or \
                (not MemoryRegion.isAligned(region_size, page_size))):
            sys.exit("Error: the region is not aligned")

    def __init__(self, start_2mb_str, end_2mb_str,
            start_1gb_str, end_1gb_str, region_size_str):
        self.start_2mb = MemoryRegion.convertSizeStringToBytes(start_2mb_str)
        self.end_2mb = MemoryRegion.convertSizeStringToBytes(end_2mb_str)
        self.start_1gb = MemoryRegion.convertSizeStringToBytes(start_1gb_str)
        self.end_1gb = MemoryRegion.convertSizeStringToBytes(end_1gb_str)
        self.region_size = MemoryRegion.convertSizeStringToBytes(region_size_str)
        self.validateRegion()

    def getNumOfLargePages(self):
        return int((self.end_2mb - self.start_2mb) / MemoryRegion.LARGE_PAGE_SIZE)

    def getNumOfHugePages(self):
        return int((self.end_1gb - self.start_1gb) / MemoryRegion.HUGE_PAGE_SIZE)

    def validateRegion(self):
        region_size_2mb = self.end_2mb - self.start_2mb
        region_size_1gb = self.end_1gb - self.start_1gb

        # validate regions not exceed pool size
        if ((self.end_2mb > self.region_size) or (self.end_1gb > self.region_size)):
            sys.exit("Error: regions exceed the pool size")

        if ((region_size_2mb > 0) and (region_size_1gb > 0)):
            # validate the delta between regions
            delta = abs(self.start_1gb - self.start_2mb)
            if (not MemoryRegion.isAligned(delta, MemoryRegion.LARGE_PAGE_SIZE)):
                sys.exit("Error: invalid delta between 1gb and 2mb offsets")
            # validate regions are not overlapping
            are_overlapping = \
                    (((self.start_1gb <= self.start_2mb) and \
                    (self.start_2mb < self.end_1gb)) or \
                    ((self.start_2mb <= self.start_1gb) and \
                    (self.start_1gb < self.end_2mb)))
            if (are_overlapping):
                sys.exit("Error: regions are overlapping")

        MemoryRegion.validateRegionOffsets(self.start_2mb, self.end_2mb,
                MemoryRegion.LARGE_PAGE_SIZE)
        MemoryRegion.validateRegionOffsets(self.start_1gb, self.end_1gb,
                MemoryRegion.HUGE_PAGE_SIZE)

import argparse
parser = argparse.ArgumentParser(description='A tool to run applications while\
         preloading mosalloc library to intercept allocation calls and\
         redirect them to preallocated regions backed with mixed pages sizes')
parser.add_argument('-z', '--analyze', action='store_true',
        help="analyze the pool sizes and write them to new file called mosalloc_hpbrs.csv.<pid>")
parser.add_argument('-d', '--debug', action='store_true',
        help="run in debug mode and don't run preparation scripts (e.g., disable THP)")
parser.add_argument('-l', '--library', default='build/src/libmosalloc.so',
        help="mosalloc library path to preload.")
parser.add_argument('-fps', '--file_pool_size', default="1GB",
        help="size of file mapping region")
parser.add_argument('-aps', '--anon_pool_size', default="16GB",
        help="size of anonymous mapping region")
parser.add_argument('-as2', '--anon_start_2mb', default="0KB",
        help="start offset of 2mb anonymous mapping region")
parser.add_argument('-ae2', '--anon_end_2mb', default="0KB",
        help="end offset of mmap 2mb region")
parser.add_argument('-as1', '--anon_start_1gb', default="0KB",
        help="start offset of mmap 1gb region")
parser.add_argument('-ae1', '--anon_end_1gb', default="0KB",
        help="end offset of mmap 1gb region")
parser.add_argument('-bps', '--brk_pool_size', default="16GB",
        help="size of brk region")
parser.add_argument('-bs2', '--brk_start_2mb', default="0KB",
        help="start offset of brk 2mb region")
parser.add_argument('-be2', '--brk_end_2mb', default="0KB",
        help="end offset of brk 2mb region")
parser.add_argument('-bs1', '--brk_start_1gb', default="0KB",
        help="start offset of brk 1gb region")
parser.add_argument('-be1', '--brk_end_1gb', default="0KB",
        help="end offset of brk 1gb region")
parser.add_argument('dispatch_program', help="program to execute")
parser.add_argument('dispatch_args', nargs=argparse.REMAINDER,
        help="program arguments")
args = parser.parse_args()

# validate the command-line arguments
if not os.path.isfile(args.library):
    sys.exit("Error: the mosalloc library cannot be found")

anon_region = MemoryRegion(args.anon_start_2mb, args.anon_end_2mb,
        args.anon_start_1gb, args.anon_end_1gb,
        args.anon_pool_size)
brk_region = MemoryRegion(args.brk_start_2mb, args.brk_end_2mb,
        args.brk_start_1gb, args.brk_end_1gb,
        args.brk_pool_size)

# build the environment variables
environ = {}
environ["HPC_MMAP_1GB_START_OFFSET"] = args.anon_start_1gb
environ["HPC_MMAP_1GB_END_OFFSET"] = args.anon_end_1gb
environ["HPC_MMAP_2MB_START_OFFSET"] = args.anon_start_2mb
environ["HPC_MMAP_2MB_END_OFFSET"] = args.anon_end_2mb
environ["HPC_MMAP_POOL_SIZE"] = args.anon_pool_size
environ["HPC_MMAP_FIRST_FIT_LIST_SIZE"] = "1MB"
environ["HPC_BRK_1GB_START_OFFSET"] = args.brk_start_1gb
environ["HPC_BRK_1GB_END_OFFSET"] = args.brk_end_1gb
environ["HPC_BRK_2MB_START_OFFSET"] = args.brk_start_2mb
environ["HPC_BRK_2MB_END_OFFSET"] = args.brk_end_2mb
environ["HPC_BRK_POOL_SIZE"] = args.brk_pool_size
environ["HPC_FILE_BACKED_POOL_SIZE"] = args.file_pool_size
environ["HPC_FILE_BACKED_FIRST_FIT_LIST_SIZE"] = "10KB"
if args.analyze:
    environ["HPC_ANALYZE_HPBRS"] = "1"
environ = {key: str(MemoryRegion.convertSizeStringToBytes(value)) \
        for key, value in environ.items()}
environ.update(os.environ)
# update the LD_PRELOAD to include our library besides others if exist
ld_preload = os.environ.get("LD_PRELOAD")
if ld_preload is None:
    environ["LD_PRELOAD"] = args.library
else:
    environ["LD_PRELOAD"] = ld_preload + ':' + args.library

# reserve an additional large/huge page so we can pad the pools with this
# extra page and allow proper alignment of large/huge pages inside the pools
large_pages= anon_region.getNumOfLargePages() + brk_region.getNumOfLargePages() + 1
huge_pages = anon_region.getNumOfHugePages() + brk_region.getNumOfHugePages() + 1

# dispatch the program with the environment we just set
import subprocess
try:
    if not args.debug:
        scripts_home_directory = sys.path[0]
        reserve_huge_pages_script = scripts_home_directory + "/reserveHugePages.sh"
        # set THP and reserve hugepages before start running the workload
        subprocess.check_call(['flock', '-x', reserve_huge_pages_script,
            reserve_huge_pages_script, '-l'+str(large_pages),
            '-h'+str(huge_pages)])

    command_line = [args.dispatch_program] + args.dispatch_args
    p = subprocess.Popen(command_line, env=environ, shell=False)
    p.wait()
    sys.exit(p.returncode)
except Exception as e:
    print("Caught an exception: " + str(e))
    print('Please make sure that you already run:')
    print('sudo bash -c "echo 1 > /proc/sys/vm/overcommit_memory"')
    print('sudo bash -c "echo never > /sys/kernel/mm/transparent_hugepage/enabled"')
    sys.exit(-1)

print("Execution completed....")

