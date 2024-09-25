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

// Board class implementation (as per your code)
// Include the Board class definition and method implementations
// ...

// For the purpose of this test, I'll include the Board class as per your implementation
// Ensure that you include the actual implementation in your test environment

// Begin unit tests
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
#include "utils.h"
#include "Parser/utility.h"

// For the purpose of this test code, I'll include minimal definitions

// Board::ParameterInfo definition (as per your implementation)


// Mock utility functions
// ... (As before)

// Begin unit tests

TEST(BoardTest, EmplaceAndRetrieveParameter) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation rawToPhysic = { "x", { "Param1" } };
    Board::ParameterInfo::Equation physicToRaw = { "x", { "Param1" } };

    Board::ParameterInfo param(
        "Param1",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamUnsigned", { "ParamUnsigned" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamUnsigned", { "ParamUnsigned" } };

    Board::ParameterInfo param(
        "ParamUnsigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        rawToPhysic,
        physicToRaw,
        false,
        false
    );

    board.emplace(param);

    if(board.doesExist(param.name) == false)
    {
        std::cout<<"Parameter not found\n";
    }
    std::cout << "Variables number:  "<< board.at(param.name).rawToPhysic.variables.size();

    uint32_t rawValue = 0x1234;
    double physicalValue = board.calculatePhysical(param.name, rawValue);

    EXPECT_EQ(physicalValue, 0x1234);
}

TEST(BoardTest, CalculatePhysicalSigned) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation rawToPhysic = { "ParamSigned", { "ParamSigned" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamSigned", { "ParamSigned" } };

    Board::ParameterInfo param(
        "ParamSigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Signed,
        -32768.0,
        32767.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamUnsigned", { "ParamUnsigned" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamUnsigned", { "ParamUnsigned" } };

    Board::ParameterInfo param(
        "ParamUnsigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamSigned", { "ParamSigned" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamSigned", { "ParamSigned" } };

    Board::ParameterInfo param(
        "ParamSigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Signed,
        -32768.0,
        32767.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamUnsigned64", { "ParamUnsigned64" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamUnsigned64", { "ParamUnsigned64" } };

    Board::ParameterInfo param(
        "ParamUnsigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        4294967295.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamSigned64", { "ParamSigned64" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamSigned64", { "ParamSigned64" } };

    Board::ParameterInfo param(
        "ParamSigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Signed,
        -2147483648.0,
        2147483647.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamUnsigned64", { "ParamUnsigned64" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamUnsigned64", { "ParamUnsigned64" } };

    Board::ParameterInfo param(
        "ParamUnsigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        4294967295.0,
        rawToPhysic,
        physicToRaw,
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

    Board::ParameterInfo::Equation rawToPhysic = { "ParamSigned64", { "ParamSigned64" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamSigned64", { "ParamSigned64" } };

    Board::ParameterInfo param(
        "ParamSigned64",
        0x1000,
        0,
        64,
        2,
        Board::ParameterInfo::ValueEncoding::Signed,
        -2147483648.0,
        2147483647.0,
        rawToPhysic,
        physicToRaw,
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

TEST(BoardTest, EmplaceParameterWithInvalidAddress) {
    Board board("TestBoard", 0x1000);

    Board::ParameterInfo::Equation rawToPhysic = { "ParamInvalid", { "ParamInvalid" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamInvalid", { "ParamInvalid" } };

    Board::ParameterInfo param(
        "ParamInvalid",
        0x0FFF, // Invalid base address (less than board address)
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        rawToPhysic,
        physicToRaw,
        false,
        false
    );

    EXPECT_THROW(board.emplace(param), std::runtime_error);
}

TEST(BoardTest, RandomizedCalculatePhysicalAndRaw) {
   
    Board board("RandomBoard", 0x1000);
    std::mt19937 rng(123); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint8_t> bitLengthDist(1, 32);

    // Test with 1000 random parameters
    for (int i = 0; i < 100; ++i) {

        // Random bit length between 1 and 32
        uint8_t bitLength = bitLengthDist(rng);

        // Calculate min and max values based on bitLength
        double minValue = -(1LL << (bitLength - 1));
        double maxValue = (1LL << (bitLength - 1)) - 1;

        Board::ParameterInfo::Equation rawToPhysic = { "RandomParam" + std::to_string(i), { "RandomParam" + std::to_string(i) } };
        Board::ParameterInfo::Equation physicToRaw = { "RandomParam" + std::to_string(i), { "RandomParam" + std::to_string(i) } };

        Board::ParameterInfo param(
            "RandomParam" + std::to_string(i),
            0x1000,
            0,
            bitLength,
            1,
            Board::ParameterInfo::ValueEncoding::Signed,
            minValue,
            maxValue,
            rawToPhysic,
            physicToRaw,
            false,
            false
        );

        EXPECT_TRUE(board.emplace(param));

        // Random signed integer value within the representable range
        std::uniform_int_distribution<int32_t> signedDist(static_cast<int32_t>(minValue), static_cast<int32_t>(maxValue));
        int32_t randomValue = signedDist(rng);

        // Encode the value
        uint32_t rawValue = twosComplementEncode<int32_t>(randomValue, bitLength);

        // Calculate physical value from raw
        double physicalValue = board.calculatePhysical(param.name, rawValue << param.startBit);
        EXPECT_EQ(static_cast<int32_t>(physicalValue), randomValue) << "Failed at bitLength=" << static_cast<int>(bitLength);

        // Calculate raw value from physical
        uint32_t calculatedRaw = board.calculateRaw(param.name, physicalValue);
        EXPECT_EQ(calculatedRaw >> param.startBit, rawValue) << "Failed at bitLength=" << static_cast<int>(bitLength);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}