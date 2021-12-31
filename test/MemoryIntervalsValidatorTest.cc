//
// Created by User on 17/11/2019.
//

#include "gtest/gtest.h"
#include "MemoryIntervalsValidator.h"
#include <MemoryIntervalListTestHelper.h>

#define GB (1073741824)
#define MB (1048576)

TEST(MemoryIntervalsValidatorTest, ValidCaseBasicTest) {

    MemoryIntervalList ls;
    MemoryIntervalsValidator validator;
    MemoryIntervalListTestHelper::setMultiplyRegions2MBFirstValidIntervalList(ls);
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseRegion2MBFirstStartFromZeroTest) {

    MemoryIntervalList ls;
    MemoryIntervalsValidator validator;
    MemoryIntervalListTestHelper::setMultiplyWindows2MBFirstStartFromZeroValidIntervalList(ls);
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseReigion1GBFirstTest) {

    MemoryIntervalList ls;
    MemoryIntervalsValidator validator;
    MemoryIntervalListTestHelper::setMultiplyWindows1GBFirstValidIntervalList(ls);
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}


TEST(MemoryIntervalsValidatorTest, ValidCaseRegion1GBFirstStartFromZeroTest) {

    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setMultiplyWindows1GBFirstStartFromZeroValidIntervalList(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseMultiplyWindowsWithout2MBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setMultiplyWindowsWithout2MBValidIntervalList(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseMultiplyWindowsOnly1GBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setMultiplyWindowsOnly1GValidIntervalList(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseMultiplyWindowsWithout1GBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setMultiplyWindowsWithout1GBValidIntervalList(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseMultiplyWindowsOnly2MBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setMultiplyWindowsOnly2MBValidIntervalList(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, ValidCaseEmptyListTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setEmptyIntervalList(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SUCCESS);
}

TEST(MemoryIntervalsValidatorTest, InvalidOffsetBetweenTwo1GBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setListInvalidOffsetBetweenTwo1GBIntervalsError(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::OFFSET_BETWEEN_TWO_1GB_INTERVALS_ERROR);
}

TEST(MemoryIntervalsValidatorTest, InvalidSizeOf1GBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setListInvalidSizeOf1GBIntervalError(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SIZE_OF_1GB_INTERVAL_ERROR);
}

TEST(MemoryIntervalsValidatorTest, NegativeSizeOf1GBTest) {
    MemoryIntervalList intervalList;
    auto start_1gb_1 = (1 * GB) +  (12 * MB);
    auto end_1gb_1 = 12 * MB;
    auto start_2mb_1 = end_1gb_1 + 12 * MB;
    auto end_2mb_1 = start_2mb_1 + 50 * MB;

    intervalList.Initialize(mmap, munmap, 2);
    intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);

    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(intervalList);
    EXPECT_EQ(result, ValidatorErrorMessage::SIZE_OF_1GB_INTERVAL_ERROR);
}

TEST(MemoryIntervalsValidatorTest, Interval1GBNotAlignedTo4KBTest) {
    MemoryIntervalList intervalList;
    auto start_1gb_1 = 3 * KB;
    auto end_1gb_1 = start_1gb_1 + 1 * GB;
    auto start_2mb_1 = end_1gb_1 + 12 * MB;
    auto end_2mb_1 = start_2mb_1 + 50 * MB;

    intervalList.Initialize(mmap, munmap, 2);
    intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);

    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(intervalList);
    EXPECT_EQ(result, ValidatorErrorMessage::INVALID_1GB_START_OFFSET);
}

TEST(MemoryIntervalsValidatorTest, InvalidOffsetBetweenTwo2MBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setListInvalidOffsetBetweenTwo2MBIntervalsError(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::OFFSET_BETWEEN_TWO_2MB_INTERVALS_ERROR);
}

TEST(MemoryIntervalsValidatorTest, InvalidSizeOf2MBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setListInvalidSizeOf2MBIntervalError(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::SIZE_OF_2MB_INTERVAL_ERROR);
}

TEST(MemoryIntervalsValidatorTest, NegativeSizeOf2MBTest) {
    MemoryIntervalList intervalList;
    auto start_1gb_1 = (12 * MB);
    auto end_1gb_1 = (1 * GB) + start_1gb_1;
    auto start_2mb_1 = end_1gb_1 + 12 * MB;
    auto end_2mb_1 = start_2mb_1 + 50 * MB;

    intervalList.Initialize(mmap, munmap, 2);
    intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    intervalList.AddInterval(end_2mb_1, start_2mb_1, PageSize::HUGE_2MB);

    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(intervalList);
    EXPECT_EQ(result, ValidatorErrorMessage::SIZE_OF_2MB_INTERVAL_ERROR);
}

TEST(MemoryIntervalsValidatorTest, Interval2MBNotAlignedTo4KBTest) {
    MemoryIntervalList intervalList;
    auto start_1gb_1 = 12 * MB;
    auto end_1gb_1 = start_1gb_1 + 1 * GB;
    auto start_2mb_1 = end_1gb_1 + 3 * KB;
    auto end_2mb_1 = start_2mb_1 + 50 * MB;

    intervalList.Initialize(mmap, munmap, 2);
    intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);

    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(intervalList);
    EXPECT_EQ(result, ValidatorErrorMessage::INVALID_2MB_START_OFFSET);
}

TEST(MemoryIntervalsValidatorTest, InvalidOffsetBetween1GBAnd2MBTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setListInvalidOffsetBetween1GBAnd2MBIntervalsError(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::OFFSET_BETWEEN_1GB_AND_2MB_INTERVALS_ERROR);
}

TEST(MemoryIntervalsValidatorTest, InvalidPageSizeTest) {
    MemoryIntervalList ls;
    MemoryIntervalListTestHelper::setListInvalidPageSizeError(ls);
    MemoryIntervalsValidator validator;
    ValidatorErrorMessage result = validator.Validate(ls);
    EXPECT_EQ(result, ValidatorErrorMessage::INVALID_PAGE_SIZE);
}

