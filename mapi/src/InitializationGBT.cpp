#include"Initialization.h"


std::string InitializerGBT::getInitSequence()
{
    std::vector<SwtSequence::SwtOperation> operations;

    operations.emplace_back(
        SwtSequence::Operation::Write,
        0xD8,
        std::array<uint32_t,2>({0x00100000}),
        true
        );

    operations.emplace_back(
        SwtSequence::Operation::Write,
        0xD9,
        std::array<uint32_t,2>({0x40}),
        true
        );
    
    for(uint32_t address = 0xDA; address <= 0xE3; address++){
        if(address == 0xE1) continue;
        operations.emplace_back(
            SwtSequence::Operation::Write,
            0xDA,
            std::array<uint32_t,2>({0x0}),
            true
            );
    }
    
    operations.emplace_back(
        SwtSequence::Operation::Write,
        0xE4,
        std::array<uint32_t,2>({0x10}),
        true
        );

    uint32_t systemID = m_board->
    operations.emplace_back(
        SwtSequence::Operation::RMWbits
    );   

}