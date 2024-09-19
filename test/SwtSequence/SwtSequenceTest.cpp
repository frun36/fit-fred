// SwtSequenceTest.cpp

#include "gtest/gtest.h"
#include "SwtSequence.h"

namespace {

TEST(SwtSequenceTest, WordToHex)
{
    EXPECT_EQ(fit_swt::SwtSequence::wordToHex(0x12345678), "12345678");
    EXPECT_EQ(fit_swt::SwtSequence::wordToHex(0xABCDEF01), "ABCDEF01");
    EXPECT_EQ(fit_swt::SwtSequence::wordToHex(0x0), "00000000");
    EXPECT_EQ(fit_swt::SwtSequence::wordToHex(0xFFFFFFFF), "FFFFFFFF");
}

TEST(SwtSequenceTest, HalfByteToHex)
{
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(0), '0');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(1), '1');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(2), '2');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(3), '3');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(4), '4');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(5), '5');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(6), '6');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(7), '7');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(8), '8');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(9), '9');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(10), 'A');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(11), 'B');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(12), 'C');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(13), 'D');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(14), 'E');
    EXPECT_EQ(fit_swt::SwtSequence::halfByteToHex(15), 'F');
}

TEST(SwtSequenceTest, CreateMask)
{
    uint32_t dest[2];
    fit_swt::SwtSequence::createMask(4, 7, 0xF, dest);
    EXPECT_EQ(dest[0], 0xFFFFFF0F);
    EXPECT_EQ(dest[1], 0x000000F0);

    fit_swt::SwtSequence::createMask(0, 31, 0xAAAAAAAA, dest);
    EXPECT_EQ(dest[0], 0x00000000);
    EXPECT_EQ(dest[1], 0xAAAAAAAA);

    fit_swt::SwtSequence::createMask(5, 5, 1, dest);
    EXPECT_EQ(dest[0], 0xFFFFFFDF);
    EXPECT_EQ(dest[1], 0x00000020);
}

TEST(SwtSequenceTest, PassMask)
{
    fit_swt::SwtSequence seq;
    const uint32_t* mask = seq.passMask(4, 7, 0xF);
    EXPECT_EQ(mask[0], 0xFFFFFF0F);
    EXPECT_EQ(mask[1], 0x000000F0);
}

TEST(SwtSequenceTest, InitialBuffer)
{
    fit_swt::SwtSequence seq;
    EXPECT_EQ(seq.getSequence(), "reset\n");
}

TEST(SwtSequenceTest, AddOperationRead)
{
    fit_swt::SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    seq.addOperation(fit_swt::SwtSequence::Operation::Read, address);
    std::string expected = "reset\n";
    std::string addr_hex = fit_swt::SwtSequence::wordToHex(address);
    expected += "0x000" + addr_hex + "0000";
    expected += ",write\n";
    expected += "read\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationWrite)
{
    fit_swt::SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    uint32_t data = 0x56789ABC;
    seq.addOperation(fit_swt::SwtSequence::Operation::Write, address, &data);
    std::string expected = "reset\n";
    std::string addr_hex = fit_swt::SwtSequence::wordToHex(address);
    std::string data_hex = fit_swt::SwtSequence::wordToHex(data);
    expected += "0x001" + addr_hex + data_hex;
    expected += ",write\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationRMWbits)
{
    fit_swt::SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    uint32_t data[2] = {0xFFFFFF0F, 0x000000F0};
    seq.addOperation(fit_swt::SwtSequence::Operation::RMWbits, address, data);
    std::string expected = "reset\n";
    std::string addr_hex = fit_swt::SwtSequence::wordToHex(address);
    std::string data0_hex = fit_swt::SwtSequence::wordToHex(data[0]);
    std::string data1_hex = fit_swt::SwtSequence::wordToHex(data[1]);
    expected += "0x002" + addr_hex + data0_hex + ",write\n";
    expected += "read\n";
    expected += "0x003" + addr_hex + data1_hex + ",write\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationRMWsum)
{
    fit_swt::SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    uint32_t data = 0x00000010;
    seq.addOperation(fit_swt::SwtSequence::Operation::RMWsum, address, &data);
    std::string expected = "reset\n";
    std::string addr_hex = fit_swt::SwtSequence::wordToHex(address);
    std::string data_hex = fit_swt::SwtSequence::wordToHex(data);
    expected += "0x004" + addr_hex + data_hex + ",write\n";
    expected += "read\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddMultipleOperations)
{
    fit_swt::SwtSequence seq;
    uint32_t address1 = 0x1234ABCD;
    uint32_t data1 = 0x56789ABC;
    uint32_t address2 = 0xABCDEF01;
    seq.addOperation(fit_swt::SwtSequence::Operation::Write, address1, &data1);
    seq.addOperation(fit_swt::SwtSequence::Operation::Read, address2);

    std::string expected = "reset\n";
    std::string addr1_hex = fit_swt::SwtSequence::wordToHex(address1);
    std::string data1_hex = fit_swt::SwtSequence::wordToHex(data1);
    std::string addr2_hex = fit_swt::SwtSequence::wordToHex(address2);
    expected += "0x001" + addr1_hex + data1_hex + ",write\n";
    expected += "0x000" + addr2_hex + "0000" + ",write\n";
    expected += "read\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationNoResponse)
{
    fit_swt::SwtSequence seq;
    uint32_t address = 0x1234ABCD;
    seq.addOperation(fit_swt::SwtSequence::Operation::Read, address, nullptr, false);
    std::string expected = "reset\n";
    std::string addr_hex = fit_swt::SwtSequence::wordToHex(address);
    expected += "0x000" + addr_hex + "0000";
    expected += ",write\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

TEST(SwtSequenceTest, AddOperationWithHexAddress)
{
    fit_swt::SwtSequence seq;
    const char* address = "1234ABCD";
    uint32_t data = 0x56789ABC;
    seq.addOperation(fit_swt::SwtSequence::Operation::Write, address, &data);
    std::string expected = "reset\n";
    std::string data_hex = fit_swt::SwtSequence::wordToHex(data);
    expected += "0x001" + std::string(address) + data_hex + ",write\n";

    EXPECT_EQ(seq.getSequence(), expected);
}

}  // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
