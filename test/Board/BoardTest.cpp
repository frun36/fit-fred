// Include necessary headers
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <random>

// Include the definitions of Board and Board::ParameterInfo
// Replace these includes with the actual paths or code
#include "Board.h"
#include "utils.h" // For twosComplementEncode, twosComplementDecode, getBitField
#include "Parser/utility.h" // For Utility::calculateEquation

TEST(BoardTest, EmplaceAndRetrieveParameter) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "x", { "Param1" } };
    Board::ParameterInfo::Equation physicToElectronic = { "x", { "Param1" } };

    Board::ParameterInfo param(
        "Param1",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    auto result = board.emplace(param);
    EXPECT_TRUE(result); // Emplace should succeed

    // Try to retrieve the parameter
    Board::ParameterInfo& retrievedParam = board.at("Param1");
    EXPECT_EQ(retrievedParam.name, "Param1");
    EXPECT_EQ(retrievedParam.baseAddress, 0x1000u);
    EXPECT_EQ(retrievedParam.startBit, 0u);
    EXPECT_EQ(retrievedParam.bitLength, 16u);
}

TEST(BoardTest, CalculatePhysicalUnsigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamUnsigned", { "ParamUnsigned" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamUnsigned", { "ParamUnsigned" } };

    Board::ParameterInfo param(
        "ParamUnsigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    if(board.doesExist(param.name) == false)
    {
        std::cout<<"Parameter not found\n";
    }
    std::cout << "Variables number:  "<< board.at(param.name).electronicToPhysic.variables.size();

    uint32_t rawValue = 0x1234;
    double physicalValue = board.calculatePhysical(param.name, rawValue);

    EXPECT_EQ(physicalValue, 0x1234);
}

TEST(BoardTest, CalculatePhysicalSigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamSigned", { "ParamSigned" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamSigned", { "ParamSigned" } };

    Board::ParameterInfo param(
        "ParamSigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Signed,
        -32768.0,
        32767.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    uint32_t rawValue = 0xFFFF; // -1 in 16-bit two's complement
    double physicalValue = board.calculatePhysical("ParamSigned", rawValue);

    EXPECT_EQ(physicalValue, -1);
}

TEST(BoardTest, CalculateRawUnsigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamUnsigned", { "ParamUnsigned" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamUnsigned", { "ParamUnsigned" } };

    Board::ParameterInfo param(
        "ParamUnsigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    double physicalValue = 0x1234;
    uint32_t rawValue = board.calculateRaw("ParamUnsigned", physicalValue);

    EXPECT_EQ(rawValue, 0x1234);
}

TEST(BoardTest, CalculateRawSigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamSigned", { "ParamSigned" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamSigned", { "ParamSigned" } };

    Board::ParameterInfo param(
        "ParamSigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Signed,
        -32768.0,
        32767.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    double physicalValue = -1;
    uint32_t rawValue = board.calculateRaw("ParamSigned", physicalValue);

    EXPECT_EQ(rawValue, 0xFFFF); // -1 in 16-bit two's complement
}

TEST(BoardTest, DISABLED_CalculatePhysical64Unsigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamUnsigned64", { "ParamUnsigned64" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamUnsigned64", { "ParamUnsigned64" } };

    Board::ParameterInfo param(
        "ParamUnsigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        4294967295.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    uint64_t rawValue = 0x12345678;
    double physicalValue = board.calculatePhysical64("ParamUnsigned64", rawValue);

    EXPECT_EQ(physicalValue, 0x12345678);
}

TEST(BoardTest, DISABLED_CalculatePhysical64Signed) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamSigned64", { "ParamSigned64" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamSigned64", { "ParamSigned64" } };

    Board::ParameterInfo param(
        "ParamSigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Signed,
        -2147483648.0,
        2147483647.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    uint64_t rawValue = 0xFFFFFFFF; // -1 in 32-bit two's complement
    double physicalValue = board.calculatePhysical64("ParamSigned64", rawValue);

    EXPECT_EQ(physicalValue, -1);
}

TEST(BoardTest, DISABLED_CalculateRaw64Unsigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamUnsigned64", { "ParamUnsigned64" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamUnsigned64", { "ParamUnsigned64" } };

    Board::ParameterInfo param(
        "ParamUnsigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        4294967295.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    double physicalValue = 0x12345678;
    uint64_t rawValue = board.calculateRaw64("ParamUnsigned64", physicalValue);

    EXPECT_EQ(rawValue, 0x12345678);
}

TEST(BoardTest, DISABLED_CalculateRaw64Signed) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamSigned64", { "ParamSigned64" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamSigned64", { "ParamSigned64" } };

    Board::ParameterInfo param(
        "ParamSigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Signed,
        -2147483648.0,
        2147483647.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    double physicalValue = -1;
    uint64_t rawValue = board.calculateRaw64("ParamSigned64", physicalValue);

    EXPECT_EQ(rawValue, 0xFFFFFFFF); // -1 in 32-bit two's complement
}

TEST(BoardTest, ParameterDoesNotExist) {
    Board board("TestBoard", 0x1000);

    // Attempt to retrieve a non-existent parameter
    EXPECT_THROW(board.at("NonExistentParam"), std::out_of_range);

    // Attempt to calculate physical value for a non-existent parameter
    EXPECT_THROW(board.calculatePhysical("NonExistentParam", 0x1234), std::runtime_error);

    // Attempt to calculate raw value for a non-existent parameter
    EXPECT_THROW(board.calculateRaw("NonExistentParam", 42.0), std::runtime_error);
}

TEST(BoardTest, ParameterWithoutStoredValue) {
    Board board("TestBoard", 0x1000);
        Board::ParameterInfo::Equation electronicToPhysic = { "ParamSigned64", { "ParamSigned64" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamSigned64", { "ParamSigned64" } };

    Board::ParameterInfo param(
        "ParamSigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Signed,
        -2147483648.0,
        2147483647.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    board.emplace(param);

    // Attempt to retrieve a non-existent parameter
    EXPECT_THROW(board.at(param.name).getStoredValue(), std::runtime_error);
}

TEST(BoardTest, EmplaceParameterWithInvalidAddress) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation electronicToPhysic = { "ParamInvalid", { "ParamInvalid" } };
    Board::ParameterInfo::Equation physicToElectronic = { "ParamInvalid", { "ParamInvalid" } };

    Board::ParameterInfo param(
        "ParamInvalid",
        0x0FFF, // Invalid base address (less than board address)
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    EXPECT_THROW(board.emplace(param), std::runtime_error);
}

TEST(BoardTest, RandomizedCalculatePhysicalAndRawWithStartBit) {
    Board board("RandomBoard", 0x1000);

    std::mt19937 rng(123); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint8_t> bitLengthDist(1, 32);
    std::uniform_int_distribution<uint8_t> startBitDist(0, 31);

    // Test with 100 random parameters
    for (int i = 0; i < 100; ++i) {
        // Random bit length between 1 and 32
        uint8_t bitLength = bitLengthDist(rng);

        // Maximum possible startBit to prevent overflow
        uint8_t maxStartBit = 32 - bitLength;
        std::uniform_int_distribution<uint8_t> adjustedStartBitDist(0, maxStartBit);
        uint8_t startBit = adjustedStartBitDist(rng);

        // Calculate min and max values based on bitLength
        int64_t minValue = -(1LL << (bitLength - 1));
        int64_t maxValue = (1LL << (bitLength - 1)) - 1;

        // Variable name
        std::string varName = "RandomParam" + std::to_string(i);

        // Define the equations
        Board::ParameterInfo::Equation electronicToPhysic = { varName + " * 2 - 1", { varName } };
        Board::ParameterInfo::Equation physicToElectronic = { "(" + varName + " + 1) / 2", { varName } };

        // Create the parameter
        Board::ParameterInfo param(
            varName,
            0x1000,
            startBit,
            bitLength,
            1,
            Board::ParameterInfo::ValueEncoding::Signed,
            static_cast<double>(minValue),
            static_cast<double>(maxValue),
            electronicToPhysic,
            physicToElectronic,
            false,
            false
        );

        EXPECT_TRUE(board.emplace(param));

        // Random signed integer value within the representable range
        std::uniform_int_distribution<int64_t> signedDist(minValue, maxValue);
        int64_t rawValue = signedDist(rng);

        // Encode the raw value
        uint32_t encodedRawValue = twosComplementEncode<int32_t>(static_cast<int32_t>(rawValue), bitLength);

        // Shift the encoded raw value according to the startBit
        uint32_t shiftedEncodedRawValue = encodedRawValue << startBit;

        // Calculate physical value from raw
        double physicalValue = board.calculatePhysical(param.name, shiftedEncodedRawValue);

        // Expected physical value: physicalValue = rawValue * 2 - 1
        double expectedPhysicalValue = static_cast<double>(rawValue) * 2.0 - 1.0;

        EXPECT_EQ(physicalValue, expectedPhysicalValue) << "Failed at bitLength=" << static_cast<int>(bitLength)
                                                        << ", startBit=" << static_cast<int>(startBit);

        // Calculate raw value from physical
        uint32_t calculatedRaw = board.calculateRaw(param.name, physicalValue);

        // Extract the raw value by shifting back
        uint32_t extractedRawValue = calculatedRaw >> startBit;

        // Expected raw value: expectedRawValue = (physicalValue + 1) / 2
        double expectedRawValueDouble = (physicalValue + 1.0) / 2.0;
        int64_t expectedRawValue = static_cast<int64_t>(expectedRawValueDouble);

        // Encode expectedRawValue
        uint32_t expectedEncodedRawValue = twosComplementEncode<int32_t>(static_cast<int32_t>(expectedRawValue), bitLength);

        EXPECT_EQ(extractedRawValue, expectedEncodedRawValue) << "Failed at bitLength=" << static_cast<int>(bitLength)
                                                              << ", startBit=" << static_cast<int>(startBit);
    }
}


#include <chrono>

TEST(BoardPerformanceTest, CalculatePhysicalAndRawTiming) {
    // Create a Board instance
    Board board("PerformanceBoard", 0x1000);

    // Define the equations (simple identity function for testing)
    Board::ParameterInfo::Equation electronicToPhysic = { "Param*2", { "Param" } };
    Board::ParameterInfo::Equation physicToElectronic = { "Param/2", { "Param" } };

    // Create a parameter
    Board::ParameterInfo param(
        "Param",
        0x1000,
        4,
        8,
        1,
        Board::ParameterInfo::ValueEncoding::Signed,
        -32768.0,
        32767.0,
        electronicToPhysic,
        physicToElectronic,
        false,
        false
    );

    // Emplace the parameter into the board
    bool emplaced = board.emplace(param);
    ASSERT_TRUE(emplaced) << "Failed to emplace parameter into the board.";

    // Variables to hold the total duration
    std::chrono::duration<double, std::milli> durationCalculatePhysical(0);
    std::chrono::duration<double, std::milli> durationCalculateRaw(0);

    // Number of iterations
    const int iterations = 10000;

    // Values to test
    uint32_t rawValue = 0x0120;
    double physicalValue = static_cast<int8_t>(rawValue>>4)*2; // 0x1234 in decimal


    // Measure time for calculatePhysical
    for (int i = 0; i < iterations; ++i) {
        auto startTime = std::chrono::high_resolution_clock::now();
        double result = board.calculatePhysical("Param", rawValue);
        auto endTime = std::chrono::high_resolution_clock::now();

        // Verify the result
        ASSERT_EQ(result, physicalValue);

        durationCalculatePhysical += endTime - startTime;
    }

    // Measure time for calculateRaw
    for (int i = 0; i < iterations; ++i) {
        auto startTime = std::chrono::high_resolution_clock::now();
        uint32_t result = board.calculateRaw("Param", physicalValue);
        auto endTime = std::chrono::high_resolution_clock::now();

        // Verify the result
        ASSERT_EQ(result, rawValue);

        durationCalculateRaw += endTime - startTime;
    }

    // Output the results
    std::cout << "Total time for calculatePhysical over " << iterations << " iterations: "
              << durationCalculatePhysical.count() << " ms" << std::endl;
    std::cout << "Average time per call: "
              << (durationCalculatePhysical.count() / iterations) << " ms" << std::endl;

    std::cout << "Total time for calculateRaw over " << iterations << " iterations: "
              << durationCalculateRaw.count() << " ms" << std::endl;
    std::cout << "Average time per call: "
              << (durationCalculateRaw.count() / iterations) << " ms" << std::endl;
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}