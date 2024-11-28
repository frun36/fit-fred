// SwtSequenceTest.cpp

#include "gtest/gtest.h"
#include "communication-utils/SwtSequence.h"
#include <array>
#include <vector>

namespace
{

TEST(SwtSequenceTest, WordToHex)
{
    EXPECT_EQ(SwtSequence::wordToHex(0x12345678), "12345678");
    EXPECT_EQ(SwtSequence::wordToHex(0xABCDEF01), "ABCDEF01");
    EXPECT_EQ(SwtSequence::wordToHex(0x0), "00000000");
    EXPECT_EQ(SwtSequence::wordToHex(0xFFFFFFFF), "FFFFFFFF");
}

TEST(SwtSequenceTest, HalfByteToHex)
{
    EXPECT_EQ(SwtSequence::halfByteToHex(0), '0');
    EXPECT_EQ(SwtSequence::halfByteToHex(1), '1');
    EXPECT_EQ(SwtSequence::halfByteToHex(2), '2');
    EXPECT_EQ(SwtSequence::halfByteToHex(3), '3');
    EXPECT_EQ(SwtSequence::halfByteToHex(4), '4');
    EXPECT_EQ(SwtSequence::halfByteToHex(5), '5');
    EXPECT_EQ(SwtSequence::halfByteToHex(6), '6');
    EXPECT_EQ(SwtSequence::halfByteToHex(7), '7');
    EXPECT_EQ(SwtSequence::halfByteToHex(8), '8');
    EXPECT_EQ(SwtSequence::halfByteToHex(9), '9');
    EXPECT_EQ(SwtSequence::halfByteToHex(10), 'A');
    EXPECT_EQ(SwtSequence::halfByteToHex(11), 'B');
    EXPECT_EQ(SwtSequence::halfByteToHex(12), 'C');
    EXPECT_EQ(SwtSequence::halfByteToHex(13), 'D');
    EXPECT_EQ(SwtSequence::halfByteToHex(14), 'E');
    EXPECT_EQ(SwtSequence::halfByteToHex(15), 'F');
}

TEST(SwtSequenceTest, CreateMask)
{
    std::array<uint32_t, 2> dest;
    SwtSequence::createMask(4, 4, 0xF, dest.data());
    EXPECT_EQ(dest[0], 0xFFFFFF0F);
    EXPECT_EQ(dest[1], 0x000000F0);

    SwtSequence::createMask(0, 32, 0xAAAAAAAA, dest.data());
    EXPECT_EQ(dest[0], 0x00000000);
    EXPECT_EQ(dest[1], 0xAAAAAAAA);

    SwtSequence::createMask(5, 1, 1, dest.data());
    EXPECT_EQ(dest[0], 0xFFFFFFDF);
    EXPECT_EQ(dest[1], 0x00000020);
}

TEST(SwtSequenceTest, PassMasks)
{
    SwtSequence seq;
    const uint32_t* mask = seq.passMasks(4, 4, 0xF);
    EXPECT_EQ(mask[0], 0xFFFFFF0F);
    EXPECT_EQ(mask[1], 0x000000F0);
}

TEST(SwtSequenceTest, InitialBuffer)
{
    SwtSequence seq;
    EXPECT_EQ(seq.getSequence(), "sc_reset");
}

TEST(SwtSequenceTest, AddOperationRead)
{
    SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    seq.addOperation(SwtSequence::Operation::Read, address);
    std::string expected = "sc_reset";
    std::string addr_hex = SwtSequence::wordToHex(address);
    expected += std::string(SwtSequence::_READ_PREFIX_) + addr_hex + "00000000";
    expected += SwtSequence::_FRAME_POSTFIX_;
    expected += SwtSequence::_READ_WORD_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationWrite)
{
    SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    uint32_t data = 0x56789ABC;
    seq.addOperation(SwtSequence::Operation::Write, address, &data, 1);
    std::string expected = "sc_reset";
    std::string addr_hex = SwtSequence::wordToHex(address);
    std::string data_hex = SwtSequence::wordToHex(data);
    expected += std::string(SwtSequence::_WRITE_PREFIX_) + addr_hex + data_hex;
    expected += SwtSequence::_FRAME_POSTFIX_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationRMWbits)
{
    SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    std::array<uint32_t, 2> data = { 0xFFFFFF0F, 0x000000F0 };
    seq.addOperation(SwtSequence::Operation::RMWbits, address, data.data());
    std::string expected = "sc_reset";
    std::string addr_hex = SwtSequence::wordToHex(address);
    std::string data0_hex = SwtSequence::wordToHex(data[0]);
    std::string data1_hex = SwtSequence::wordToHex(data[1]);
    expected += std::string(SwtSequence::_RMW_BITS_AND_PREFIX_) + addr_hex + data0_hex + SwtSequence::_FRAME_POSTFIX_;
    expected += SwtSequence::_READ_WORD_;
    expected += std::string(SwtSequence::_RMW_BITS_OR_PREFIX_) + addr_hex + data1_hex + SwtSequence::_FRAME_POSTFIX_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationRMWsum)
{
    SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    uint32_t data = 0x00000010;
    seq.addOperation(SwtSequence::Operation::RMWsum, address, &data);
    std::string expected = "sc_reset";
    std::string addr_hex = SwtSequence::wordToHex(address);
    std::string data_hex = SwtSequence::wordToHex(data);
    expected += std::string(SwtSequence::_RMW_SUM_PREFIX_) + addr_hex + data_hex + SwtSequence::_FRAME_POSTFIX_;
    expected += SwtSequence::_READ_WORD_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddMultipleOperations)
{
    SwtSequence seq;
    uint32_t address1 = 0x1234ABCD;
    uint32_t data1 = 0x56789ABC;
    uint32_t address2 = 0xABCDEF01;
    seq.addOperation(SwtSequence::Operation::Write, address1, &data1);
    seq.addOperation(SwtSequence::Operation::Read, address2);

    std::string expected = "sc_reset";
    std::string addr1_hex = SwtSequence::wordToHex(address1);
    std::string data1_hex = SwtSequence::wordToHex(data1);
    expected += std::string(SwtSequence::_WRITE_PREFIX_) + addr1_hex + data1_hex + SwtSequence::_FRAME_POSTFIX_;
    std::string addr2_hex = SwtSequence::wordToHex(address2);
    expected += std::string(SwtSequence::_READ_PREFIX_) + addr2_hex + "00000000" + SwtSequence::_FRAME_POSTFIX_;
    expected += SwtSequence::_READ_WORD_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationNoResponse)
{
    SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    seq.addOperation(SwtSequence::Operation::Read, address, nullptr, false);
    std::string expected = "sc_reset";
    std::string addr_hex = SwtSequence::wordToHex(address);
    expected += std::string(SwtSequence::_READ_PREFIX_) + addr_hex + "00000000";
    expected += SwtSequence::_FRAME_POSTFIX_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationWithHexAddress)
{
    SwtSequence seq;
    const char* address = "1234ABCD";
    uint32_t data = 0x56789ABC;
    seq.addOperation(SwtSequence::Operation::Write, address, &data);
    std::string expected = "sc_reset";
    std::string data_hex = SwtSequence::wordToHex(data);
    expected += std::string(SwtSequence::_WRITE_PREFIX_) + std::string(address) + data_hex + SwtSequence::_FRAME_POSTFIX_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, SwtOperationConstructor)
{
    SwtSequence::SwtOperation op(SwtSequence::Operation::Write, 0x1234ABCD, { 0x56789ABC, 0x0 }, false);
    EXPECT_EQ(op.type, SwtSequence::Operation::Write);
    EXPECT_EQ(op.address, 0x1234ABCD);
    EXPECT_EQ(op.data[0], 0x56789ABC);
    EXPECT_EQ(op.data[1], 0x0);
    EXPECT_EQ(op.expectResponse, false);
}

TEST(SwtSequenceTest, AddOperationWithSwtOperation)
{
    SwtSequence seq;
    SwtSequence::SwtOperation op(SwtSequence::Operation::Write, 0x1234ABCD, { 0x56789ABC, 0x0 }, false);
    seq.addOperation(op);

    std::string expected = "sc_reset";
    std::string addr_hex = SwtSequence::wordToHex(op.address);
    std::string data_hex = SwtSequence::wordToHex(op.data[0]);
    expected += std::string(SwtSequence::_WRITE_PREFIX_) + addr_hex + data_hex + SwtSequence::_FRAME_POSTFIX_;

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, ConstructorWithOperationsVector)
{
    std::vector<SwtSequence::SwtOperation> operations;
    operations.emplace_back(SwtSequence::Operation::Write, 0x1234ABCD, std::array<uint32_t, 2>{ 0x56789ABC, 0x0 }, false);
    operations.emplace_back(SwtSequence::Operation::RMWsum, 0xABCDEF01, std::array<uint32_t, 2>{ 0x00000010, 0x0 }, true);

    SwtSequence seq(operations);

    std::string expected = "sc_reset";
    std::string addr1_hex = SwtSequence::wordToHex(0x1234ABCD);
    std::string data1_hex = SwtSequence::wordToHex(0x56789ABC);
    expected += std::string(SwtSequence::_WRITE_PREFIX_) + addr1_hex + data1_hex + SwtSequence::_FRAME_POSTFIX_;

    std::string addr2_hex = SwtSequence::wordToHex(0xABCDEF01);
    std::string data2_hex = SwtSequence::wordToHex(0x00000010);
    expected += std::string(SwtSequence::_RMW_SUM_PREFIX_) + addr2_hex + data2_hex + SwtSequence::_FRAME_POSTFIX_;
    expected += SwtSequence::_READ_WORD_;

    EXPECT_EQ(seq.getSequence(), expected);
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
