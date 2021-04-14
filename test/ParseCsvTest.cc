
#include <iostream>
#include <sys/mman.h>
#include <fstream>

#include "gtest/gtest.h"
#include "ParseCsv.h"

std::string excel_data = "type, page size,start offset,end offset\n"
                         "mmap,2097152,0,16384\n"
                         "mmap,2097152,524288,1048576\n"
                         "mmap,2097152,1048576,16777216\n"
                         "brk,2097152,0,16384\n"
                         "mmap,-1,0,50\n"
                         "mmap,2097152,16384,524288\n"
                         "mmap,2097152,274877906944,1099511627776\n"
                         "fff,2097152,68719476736,274877906944\n"
                         "mmap,2097152,17179869184,68719476736\n"
                         "mmap,2097152,4294967296,17179869184\n"
                         "mmap,2097152,1073741824,4294967296\n"
                         "mmap,2097152,16777216,1073741824\n"
                         "mmap,2097152,68719476736,274877906944\n"
                         "brk,2097152,0,16384\n";

TEST(ParseCsvTest, CsvFiles) {
    std::ofstream myfile;
    myfile.open ("csv_file_for_test.csv", std::ios::out);
    myfile << excel_data;
    myfile.close();

    parseCsv pc;
    PoolConfigurationData l;
    l.intervalList.Initialize(mmap, munmap, 1024);
    std::string cwd(get_current_dir_name());
    std::string file = cwd + "/" + "csv_file_for_test.csv";
    std::string mmap = "mmap";
    pc.ParseCsv(l, file.c_str(), mmap.c_str());

    EXPECT_EQ(l.intervalList.At(0)._start_offset, (0));
    EXPECT_EQ(l.intervalList.At(1)._start_offset, (1 << 14)); //14
    EXPECT_EQ(l.intervalList.At(2)._start_offset, (1 << 19)); //19
    EXPECT_EQ(l.intervalList.At(3)._start_offset, (1 << 20)); //20
    EXPECT_EQ(l.intervalList.At(4)._start_offset, (1 << 24));//24
    EXPECT_EQ(l.intervalList.At(5)._start_offset, (1ul << 30));//30
    EXPECT_EQ(l.intervalList.At(6)._start_offset, (1ul << 32));//32
    EXPECT_EQ(l.intervalList.At(7)._start_offset, (1ul << 34)); //34
    EXPECT_EQ(l.intervalList.At(8)._start_offset, (1ul << 36));//36
    EXPECT_EQ(l.intervalList.At(9)._start_offset, (1ul << 38));//38

    EXPECT_EQ(l.intervalList.At(0)._end_offset, (1 << 14)); //14
    EXPECT_EQ(l.intervalList.At(1)._end_offset, (1 << 19)); //19
    EXPECT_EQ(l.intervalList.At(2)._end_offset, (1 << 20)); //20
    EXPECT_EQ(l.intervalList.At(3)._end_offset, (1 << 24));//24
    EXPECT_EQ(l.intervalList.At(4)._end_offset, (1ul << 30));//30
    EXPECT_EQ(l.intervalList.At(5)._end_offset, (1ul << 32));//32
    EXPECT_EQ(l.intervalList.At(6)._end_offset, (1ul << 34)); //34
    EXPECT_EQ(l.intervalList.At(7)._end_offset, (1ul << 36));//36
    EXPECT_EQ(l.intervalList.At(8)._end_offset, (1ul << 38));//38
    EXPECT_EQ(l.intervalList.At(9)._end_offset, (1ul << 40));//38
    EXPECT_EQ(l.size, 50);//38
    remove("csv_file_for_test.csv");
}