// Include necessary headers
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <random>
#include"Board.h"

// Begin extended unit tests

// Test for exceptions and random testing

TEST(BoardTest, ExceptionHandling) {
    Board board("TestBoard", 0x1000);

    // Attempt to retrieve a non-existent parameter
    EXPECT_THROW(board.get("NonExistentParam"), std::runtime_error);

    // Attempt to add a parameter with an invalid address
    ParameterInfo invalidParam("InvalidParam", 0x0FFF, 0, 16);
    EXPECT_THROW(board.emplace(invalidParam), std::runtime_error);

    // Attempt to calculate physical value for a non-existent parameter
    EXPECT_THROW(board.calculatePhysical("NonExistentParam", 0x1234), std::runtime_error);

    // Attempt to calculate raw value for a non-existent parameter
    EXPECT_THROW(board.calculateRaw("NonExistentParam", 42.0), std::runtime_error);
}

TEST(BoardTest, RandomizedParameterInsertionAndRetrieval) {
    Board board("RandomBoard", 0x1000);

    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint32_t> addressDist(0x1000, 0x1FFF);
    std::uniform_int_distribution<uint8_t> bitDist(0, 31);
    std::uniform_int_distribution<uint8_t> lengthDist(1, 32);
    std::uniform_int_distribution<int32_t> valueDist(-1000000, 1000000);

    // Insert random parameters
    for (int i = 0; i < 100; ++i) {
        uint32_t baseAddress = addressDist(rng);
        uint8_t startBit = bitDist(rng);
        uint8_t bitLength = lengthDist(rng);
        ParameterInfo::ValueEncoding encoding = (i % 2 == 0) ? ParameterInfo::ValueEncoding::Unsigned : ParameterInfo::ValueEncoding::Signed;
        std::string paramName = "Param" + std::to_string(i);

        ParameterInfo param(paramName, baseAddress, startBit, bitLength, encoding);
        param.rawToPhysic.variables = { paramName };
        param.rawToPhysic.equation = "x"; // Identity function

        EXPECT_NO_THROW(board.emplace(param));
        EXPECT_TRUE(board.doesExist(paramName));

        // Retrieve the parameter and verify properties
        ParameterInfo& retrievedParam = board.get(paramName);
        EXPECT_EQ(retrievedParam.name, paramName);
        EXPECT_EQ(retrievedParam.baseAddress, baseAddress);
        EXPECT_EQ(retrievedParam.startBit, startBit);
        EXPECT_EQ(retrievedParam.bitLength, bitLength);
        EXPECT_EQ(retrievedParam.valueEncoding, encoding);
    }
}

TEST(BoardTest, RandomizedCalculatePhysicalAndRaw) {
    Board board("RandomCalcBoard", 0x1000);

    std::mt19937 rng(123); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint8_t> bitLengthDist(1, 32);
    std::uniform_int_distribution<int32_t> intValueDist(-1000000, 1000000);
    std::uniform_int_distribution<uint32_t> uintValueDist(0, 0xFFFFFFFF);

    // Add a parameter to the board
    ParameterInfo param("RandomParam", 0x1000, 0, 32, ParameterInfo::ValueEncoding::Signed);
    param.rawToPhysic.variables = { "RandomParam" };
    param.rawToPhysic.equation = "x"; // Identity function
    board.emplace(param);

    // Test calculatePhysical and calculateRaw in a loop
    for (int i = 0; i < 1000; ++i) {
        // Random bit length between 1 and 32
        uint32_t bitLength = bitLengthDist(rng);
        param.bitLength = static_cast<uint8_t>(bitLength);

        // Random signed integer value within the representable range
        int32_t minValue = -(1 << (bitLength - 1));
        int32_t maxValue = (1 << (bitLength - 1)) - 1;
        std::uniform_int_distribution<int32_t> signedDist(minValue, maxValue);
        int32_t randomValue = signedDist(rng);

        // Encode the value
        uint32_t rawValue = twosComplementEncode(randomValue, bitLength);

        // Calculate physical value from raw
        double physicalValue = board.calculatePhysical("RandomParam", rawValue << param.startBit);
        EXPECT_EQ(static_cast<int32_t>(physicalValue), randomValue);

        // Calculate raw value from physical
        uint32_t calculatedRaw = board.calculateRaw("RandomParam", physicalValue);
        EXPECT_EQ(calculatedRaw >> param.startBit, rawValue);
    }
}

TEST(BoardTest, RandomizedExceptionHandling) {
    Board board("RandomExceptionBoard", 0x1000);

    std::mt19937 rng(456); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> choiceDist(0, 1);

    // Randomly attempt to retrieve or calculate with non-existent parameters
    for (int i = 0; i < 100; ++i) {
        std::string paramName = "NonExistentParam" + std::to_string(i);

        int choice = choiceDist(rng);
        if (choice == 0) {
            // Attempt to retrieve
            EXPECT_THROW(board.get(paramName), std::runtime_error);
        } else {
            // Attempt to calculate
            EXPECT_THROW(board.calculatePhysical(paramName, 0x1234), std::runtime_error);
            EXPECT_THROW(board.calculateRaw(paramName, 42.0), std::runtime_error);
        }
    }
}

TEST(BoardTest, RandomizedBitFieldOperations) {
    std::mt19937 rng(789); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint32_t> wordDist(0, 0xFFFFFFFF);
    std::uniform_int_distribution<uint8_t> bitPosDist(0, 31);

    // Test getBitField with random inputs
    for (int i = 0; i < 1000; ++i) {
        uint32_t word = wordDist(rng);
        uint8_t first = bitPosDist(rng);
        uint8_t last = bitPosDist(rng);
        if (first > last) std::swap(first, last);

        uint32_t bitField = getBitField(word, first, last);

        // Verify the extracted bits
        uint32_t expected = (word >> first) & ((1u << (last - first + 1u)) - 1u);
        EXPECT_EQ(bitField, expected);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}