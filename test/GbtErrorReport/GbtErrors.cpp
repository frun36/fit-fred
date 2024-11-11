// SwtSequenceTest.cpp

#include "gtest/gtest.h"
#include "gbtInterfaceUtils.h"
#include <array>
#include <vector>
#include<cstdint>
#include<array>
#include<fstream>
#include<cstring>

namespace
{

uint32_t BcSynLostData [] = {
    0xEEEE000A, 
    0xFB8F1169, 0xAE88C165, 0x085F67C2,
    0x11203040, 0x50607080, 0x085EA0B0,
    0x12203040, 0x50607080, 0x085D085E,
    0x13203040, 0x50607080, 0x085C085D,
    0x14203040, 0x50607080, 0x085B085C,
    0x15203040, 0x50607080, 0x085A085F, 
    0x16203040, 0x50607080, 0x0859085F,
    0x17203040, 0x50607080, 0x0858085F,
    0x18203040, 0x50607080, 0x0857085F,
    0x19203040, 0x50607080, 0x0856085F,  

    0x0E850038, 0x2A3DF2C8, 0x96386010,
    0x00112233, 0x33221100
    };

uint32_t PmEarlyHeaderData [] = {
    0xEEEE000A, 
    0xFB8F1169, 0xAE88C165, 0x085F67C2,
    0x11203040, 0x50607080, 0x085EA0B0,
    0x12203040, 0x50607080, 0x085D085E,
    0x13203040, 0x50607080, 0x085C085D,
    0x14203040, 0x50607080, 0x085B085C,
    0x15203040, 0x50607080, 0x085A085F, 
    0x16203040, 0x50607080, 0x0859085F,
    0x17203040, 0x50607080, 0x0858085F,
    0x18203040, 0x50607080, 0x0857085F,
    0x19203040, 0x50607080, 0x0856085F,  

    0x0E850038, 0x2A3DF2C8, 0x96386010,
    0x00112233, 0x33221100
    };

uint32_t FifoOverloadData [] = {
    0xEEEE000A, 
    0xFB8F1169, 0xAE88C165, 0x085F67C2,
    0x11203040, 0x50607080, 0x085EA0B0,
    0x12203040, 0x50607080, 0x085D085E,
    0x13203040, 0x50607080, 0x085C085D,
    0x14203040, 0x50607080, 0x085B085C,
    0x15203040, 0x50607080, 0x085A085F, 
    0x16203040, 0x50607080, 0x0859085F,
    0x17203040, 0x50607080, 0x0858085F,
    0x18203040, 0x50607080, 0x0857085F,
    0x19203040, 0x50607080, 0x0856085F,  

    0x0E850038, 0x2A3DF2C8, 0x96386010,
    0x00112233, 0x33221100
    };

uint32_t UnknownData [] = {
    0xEEEE000A, 
    0xFB8F1169, 0xAE88C165, 0x085F67C2,
    0x11203040, 0x50607080, 0x085EA0B0,
    0x12203040, 0x50607080, 0x085D085E,
    0x13203040, 0x50607080, 0x085C085D,
    0x14203040, 0x50607080, 0x085B085C,
    0x15203040, 0x50607080, 0x085A085F, 
    0x16203040, 0x50607080, 0x0859085F,
    0x17203040, 0x50607080, 0x0858085F,
    0x18203040, 0x50607080, 0x0857085F,
    0x19203040, 0x50607080, 0x0856085F,  

    0x0E850038, 0x2A3DF2C8, 0x96386010,
    0x00112233, 0x33221100
    };

TEST(GbtErrorReport, BcSyncLost)
{
    std::array<uint32_t, gbt::constants::FifoSize> data;
    std::memcpy(data.data(), BcSynLostData, sizeof(BcSynLostData));
    
    std::cout << std::endl;
    gbt::BCSyncLost error(data);

    ASSERT_NO_THROW(error.saveErrorReport());

    std::ifstream file;
    file.open(gbt::GbtErrorType::ErrorFile.data());
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    // Close the file
    file.close();
    std::remove(gbt::GbtErrorType::ErrorFile.data());
}

TEST(GbtErrorReport, PmEarlyHeader)
{
    std::array<uint32_t, gbt::constants::FifoSize> data;
    std::memcpy(data.data(), PmEarlyHeaderData, sizeof(PmEarlyHeaderData));
    
    std::cout << std::endl;
    gbt::PmEarlyHeader error(data);

    ASSERT_NO_THROW(error.saveErrorReport());

    std::ifstream file;
    file.open(gbt::GbtErrorType::ErrorFile.data());
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    // Close the file
    file.close();
    std::remove(gbt::GbtErrorType::ErrorFile.data());
}

TEST(GbtErrorReport, FifoOverload)
{
    std::array<uint32_t, gbt::constants::FifoSize> data;
    std::memcpy(data.data(), FifoOverloadData, sizeof(FifoOverloadData));
    
    std::cout << std::endl;
    gbt::FifoOverload error(data);

    ASSERT_NO_THROW(error.saveErrorReport());

    std::ifstream file;
    file.open(gbt::GbtErrorType::ErrorFile.data());
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    // Close the file
    file.close();
    std::remove(gbt::GbtErrorType::ErrorFile.data());
}

TEST(GbtErrorReport, Unknown)
{
    std::array<uint32_t, gbt::constants::FifoSize> data;
    std::memcpy(data.data(), UnknownData, sizeof(UnknownData));
    
    std::cout << std::endl;
    gbt::Unknown error(data);

    ASSERT_NO_THROW(error.saveErrorReport());

    std::ifstream file;
    file.open(gbt::GbtErrorType::ErrorFile.data());
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    // Close the file
    file.close();
    std::remove(gbt::GbtErrorType::ErrorFile.data());
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
