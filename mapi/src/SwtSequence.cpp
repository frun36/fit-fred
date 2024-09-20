#include "SwtSequence.h"
#include<cstring>
#include<charconv>


SwtSequence& SwtSequence::addOperation(Operation type, uint32_t address, const uint32_t* data, bool expectResponse)
{
    std::string saddress = wordToHex(address);
    return addOperation(type, saddress.c_str(), data, expectResponse);
}

SwtSequence& SwtSequence::addOperation(Operation type, const char* address, const uint32_t* data, bool expectResponse)
{
    switch(type)
    {
        case Operation::Read:
        {
            m_buffer.append(_READ_PREFIX_);
            m_buffer.append(address);
            m_buffer.append("0000");
            m_buffer.append(_FRAME_POSTFIX_);

            if(expectResponse)
            {
                m_buffer.append(_READ_WORD_);
            }
        }
        break;

        case Operation::Write:
        {
            m_buffer.append(_WRITE_PREFIX_);
            m_buffer.append(address);
            m_buffer.append(wordToHex(data[0]));
            m_buffer.append(_FRAME_POSTFIX_);

        }
        break;

        case Operation::RMWbits:
        {
            m_buffer.append(_RMW_BITS_AND_PREFIX_);
            m_buffer.append(address);
            m_buffer.append(wordToHex(data[0]));
            m_buffer.append(_FRAME_POSTFIX_);

            if(expectResponse)
            {
                m_buffer.append(_READ_WORD_);
            }

            m_buffer.append(_RMW_BITS_OR_PREFIX_);
            m_buffer.append(address);
            m_buffer.append(wordToHex(data[1]));
            m_buffer.append(_FRAME_POSTFIX_);
        }

        break;

        case Operation::RMWsum:
        {
            m_buffer.append(_RMW_SUM_PREFIX_);
            m_buffer.append(address);
            m_buffer.append(wordToHex(data[0]));
            m_buffer.append(_FRAME_POSTFIX_);

            if(expectResponse)
            {
                m_buffer.append(_READ_WORD_);
            }
        }
        break;
    }

    return *this;
}

SwtSequence& SwtSequence::addOperation(SwtSequence::SwtOperation&& operation)
{
    addOperation(operation.type, operation.address, operation.data.data(), operation.expectResponse);
}

SwtSequence& SwtSequence::addOperation(const SwtSequence::SwtOperation& operation)
{
    addOperation(operation.type, operation.address, operation.data.data(), operation.expectResponse);
}

std::string SwtSequence::wordToHex(uint32_t word)
{
    return {
        halfByteToHex((word >> 28) & 0x0F),
        halfByteToHex((word >> 24) & 0x0F),


        halfByteToHex((word >> 20 )& 0x0F),
        halfByteToHex((word >> 16) & 0x0F),


        halfByteToHex((word >> 12) & 0x0F),
        halfByteToHex((word >> 8) & 0x0F),

        halfByteToHex((word >> 4) & 0x0F),
        halfByteToHex(word & 0x0F)
    };
}

void SwtSequence::createMask(uint32_t firstBit, uint32_t lastBit, uint32_t value, uint32_t* dest)
{
    dest[0] = ~( ( 0xFFFFFFFFu >> ( 32 - (lastBit - firstBit + 1) ) ) << firstBit );
    dest[1] = (value << firstBit) & (~dest[0]);
}

const uint32_t* SwtSequence::passMasks(uint32_t firstBit, uint32_t lastBit, uint32_t value) 
{
    createMask(firstBit, lastBit, value, m_mask);
    return m_mask;
}
