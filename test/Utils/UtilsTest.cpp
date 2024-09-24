// Include necessary headers
#include <gtest/gtest.h>
#include <cstdint>
#include <random>
#include <limits>
#include <cmath>
#include"utils.h"

TEST(TwosComplementTest, EncodeDecode)
{
    // Create a random number generator
    std::mt19937 rng(12345); // Fixed seed for reproducibility

    // Test bitsNumber from 1 to 32
    for (uint32_t bitsNumber = 1; bitsNumber <= 32; ++bitsNumber)
    {
        int32_t minValue, maxValue;

        if (bitsNumber == 32)
        {
            minValue = std::numeric_limits<int32_t>::min();
            maxValue = std::numeric_limits<int32_t>::max();
        }
        else
        {
            minValue = static_cast<int32_t>(-(1u << (bitsNumber - 1u)));
            maxValue = static_cast<int32_t>((1u << (bitsNumber - 1u)) - 1u);
        }

        std::uniform_int_distribution<int32_t> dist(minValue, maxValue);

        // Test with 100 random values for each bitsNumber
        for (int i = 0; i < 100; ++i)
        {
            int32_t originalValue = dist(rng);

            uint32_t encodedValue = twosComplementEncode<int32_t>(originalValue, bitsNumber);
            int32_t decodedValue = twosComplementDecode<int32_t>(encodedValue, bitsNumber);

            ASSERT_EQ(decodedValue, originalValue) << "Failed at bitsNumber=" << bitsNumber << ", value=" << originalValue;
        }
    }
}

TEST(GetBitFieldTest, RandomTests)
{
    // Create a random number generator
    std::mt19937 rng(54321); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint32_t> distWord(0, std::numeric_limits<uint32_t>::max());

    // Test with 1000 random words
    for (int i = 0; i < 1000; ++i)
    {
        uint32_t word = distWord(rng);

        // Generate random first and last positions
        uint8_t first = std::uniform_int_distribution<uint8_t>(0, 31)(rng);
        uint8_t last = std::uniform_int_distribution<uint8_t>(first, 31)(rng); // Ensure last >= first
        uint32_t bitWidth = last - first + 1u;

        uint32_t extractedBits = getBitField(word, first, bitWidth);

        if (bitWidth == 32u)
        {
            ASSERT_EQ(extractedBits, word) << "Failed at word=" << word << ", first=" << static_cast<int>(first) << ", last=" << static_cast<int>(last);
        }
        else
        {
            uint32_t mask = ((1u << bitWidth) - 1u) << first;
            uint32_t expectedBits = (word & mask) >> first;

            ASSERT_EQ(extractedBits, expectedBits) << "Failed at word=" << word << ", first=" << static_cast<int>(first) << ", last=" << static_cast<int>(last);
        }
    }
}

TEST(MaxUINTTest, CorrectValues)
{
    for (uint8_t first = 0; first <= 31; ++first)
    {
        for (uint8_t last = first; last <= 31; ++last)
        {
            double maxUintValue = maxUINT(first, last);
            uint32_t bitWidth = last - first + 1u;

            double expectedMax;
            if (bitWidth == 32u)
            {
                expectedMax = static_cast<double>(UINT32_MAX);
            }
            else
            {
                expectedMax = static_cast<double>((1u << bitWidth) - 1u);
            }

            ASSERT_EQ(maxUintValue, expectedMax) << "Failed at first=" << static_cast<int>(first) << ", last=" << static_cast<int>(last);
        }
    }
}

