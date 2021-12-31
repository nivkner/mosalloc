#! /usr/bin/env python3

import sys
import os
import argparse
import subprocess

ANON_REGION_TYPE = 'mmap'
BRK_REGION_TYPE = 'brk'


def parse_arguments():
    parser = argparse.ArgumentParser(description='A tool to run applications while\
             pre-loading mosalloc library to intercept allocation calls and\
             redirect them to pre-allocated regions backed with mixed pages sizes')
    parser.add_argument('-z', '--analyze', action='store_true',
                        help="analyze the pool sizes and write them to new file called mosalloc_hpbrs.csv.<pid>")
    parser.add_argument('-d', '--debug', action='store_true',
                        help="run in debug mode and don't run preparation scripts (e.g., disable THP)")
    parser.add_argument('-l', '--library', default='src/morecore/lib_morecore.so',
                        help="mosalloc library path to preload.")
    parser.add_argument('-cpf', '--configuration_pools_file', required=True,
                        help="path to csv file with pools configuration")
    parser.add_argument('dispatch_program', help="program to execute")
    parser.add_argument('dispatch_args', nargs=argparse.REMAINDER,
                        help="program arguments")
    args = parser.parse_args()

    # validate the command-line arguments
    if not os.path.isfile(args.library):
        sys.exit("Error: the mosalloc library cannot be found")
    if not os.path.isfile(args.configuration_pools_file):
        sys.exit("Error: the configuration pools file cannot be found")

    return args


def update_config_file():
    config_file = 'configuration.txt'
    current_config = ' '.join(sys.argv[1:]) + '\n'
    import os.path
    if os.path.isfile(config_file):
        with open(config_file, 'r') as file:
            config_file_data = file.readlines()
        config_file_data = ' '.join(config_file_data)
        if not current_config == config_file_data:
            raise RuntimeError(
                'existing configuration file is different than current one:\n'
                + 'existing configuration file: ' + config_file_data + '\n'
                + 'current run configuration: ' + current_config)
        else:
            print('current configuration already exists and identical, skipping running again')
            sys.exit(0)
    else:
        with open(config_file, 'w+') as file:
            file.write(current_config)


def run_benchmark(environ):
    # reserve an additional large/huge page so we can pad the pools with this
    # extra page and allow proper alignment of large/huge pages inside the pools
    large_pages = anon_region.get_num_of_large_pages() + brk_region.get_num_of_large_pages() + 1
    huge_pages = anon_region.get_num_of_huge_pages() + brk_region.get_num_of_huge_pages() + 1

    try:
        if not args.debug:
            scripts_home_directory = sys.path[0]
            reserve_huge_pages_script = scripts_home_directory + "/reserveHugePages.sh"
            # set THP and reserve hugepages before start running the workload
            subprocess.check_call(['flock', '-x', reserve_huge_pages_script,
                                   reserve_huge_pages_script, '-l' + str(large_pages),
                                   '-h' + str(huge_pages)])

        command_line = [args.dispatch_program] + args.dispatch_args
        p = subprocess.Popen(command_line, env=environ, shell=False)
        p.wait()
        sys.exit(p.returncode)
    except Exception as e:
        #print("Caught an exception: " + str(e))
        #print('Please make sure that you already run:')
        #print('sudo bash -c "echo 1 > /proc/sys/vm/overcommit_memory"')
        #print('sudo bash -c "echo never > /sys/kernel/mm/transparent_hugepage/enabled"')
        raise e

args = parse_arguments()

from memory_region import MemoryRegion, convert_size_string_to_bytes

anon_region = MemoryRegion(args.configuration_pools_file, ANON_REGION_TYPE)
brk_region = MemoryRegion(args.configuration_pools_file, BRK_REGION_TYPE)

# build the environment variables
environ = {"HPC_CONFIGURATION_FILE": args.configuration_pools_file,
           "HPC_MMAP_FIRST_FIT_LIST_SIZE": str(convert_size_string_to_bytes("1MB")),
           "HPC_FILE_BACKED_FIRST_FIT_LIST_SIZE": str(convert_size_string_to_bytes("10KB"))}

if args.analyze:
    environ["HPC_ANALYZE_HPBRS"] = "1"

environ.update(os.environ)

# update the LD_PRELOAD to include our library besides others if exist
ld_preload = os.environ.get("LD_PRELOAD")
if ld_preload is None:
    environ["LD_PRELOAD"] = args.library
else:
    environ["LD_PRELOAD"] = ld_preload + ':' + args.librarySS

# check if configuration.txt file exists and if so compare with current parameters
#update_config_file()  #TODO: Alon&Yaron code, should be tested

# dispatch the program with the environment we just set
run_benchmark(environ)
