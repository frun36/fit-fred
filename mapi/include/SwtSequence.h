#pragma once

#include<string>
#include<cstdint>
namespace fit_swt
{


/*
    Creates SWT sequence in the ALF-ready format */
class SwtSequence
{
    public:
    /*  
        SWT operation type  */
    enum class Operation{Read, Write, RMWbits, RMWsum};

    SwtSequence():m_buffer("reset\n"){}

    /*  
        Adds operation to sequence.
        Arguments:
            - SwtSequence::Operation type - operation type (Read, Write, RMWbits, RMWsum)
            - uint32_t address - register address
            - uint32_t* data - data (value for write, AND mask and OR mask for RMWbits, value for RMWsum)
            - bool expectResponse - if true, "read" is appended to sequence
        Return:
            - Reference to itself
    */
    SwtSequence& addOperation(Operation type, uint32_t address, const uint32_t* data = nullptr, bool expectResponse=true);
    /*  
        Adds operation to sequence.
        Arguments:
            - SwtSequence::Operation type - operation type (Read, Write, RMWbits, RMWsum)
            - const char* addressHex - register address in hexadecimal format (without "0x")
            - uint32_t* data - data (value for write, AND mask and OR mask for RMWbits, value for RMWsum)
            - bool expectResponse - if true, "read" is appended to sequence
        Return:
            - Reference to itself   
    */
    SwtSequence& addOperation(Operation type, const char* address, const uint32_t* data = nullptr, bool expectResponse=true);

    /* 
        Returns stored sequence */
    const std::string& getSequence() const { return m_buffer; }

    /*
        Creates mask for RMW bits operation */
    static void createMask(uint32_t firtBit, uint32_t lastBit, uint32_t value, uint32_t* dest);

    /*
        Creates mask in an internal buffer and returns pointer to it. */
    const uint32_t* passMask(uint32_t firtBit, uint32_t lastBit, uint32_t value);

    /*
        Translate word to hex format */
    static std::string wordToHex(uint32_t word);

    /*
        Translate half byte to hex format. Inline.*/
    static char halfByteToHex(uint8_t halfByte)
    {
        switch (halfByte) {
        case 0:
            return '0';
        case 1:
            return '1';
        case 2:
            return '2';
        case 3:
            return '3';
        case 4:
            return '4';
        case 5:
            return '5';
        case 6:
            return '6';
        case 7:
            return '7';
        case 8:
            return '8';
        case 9:
            return '9';
        case 10:
            return 'A';
        case 11:
            return 'B';
        case 12:
            return 'C';
        case 13:
            return 'D';
        case 14:
            return 'E';
        case 15:
        return 'F';
        }
    }

    

    /*
        Prefix addded at the begging of every sequence  */
    static constexpr const char* _SEQUENCE_PREFIX_ = "reset";
    /* 
        CRU special word, appended to every SWT frame   */
    static constexpr const char* _FRAME_POSTFIX_ = ",write\n";
    /* 
        CRU special word, appended to sequence if response to current operation (one SWT frame) is expected */
    static constexpr const char* _READ_WORD_ = "read\n";
    /* 

    /*
        FIT SWT specific, marks read operation  */
    static constexpr const char* _READ_PREFIX_ = "0x000";
    /*
        FIT SWT specific, marks write operation */
    static constexpr const char* _WRITE_PREFIX_ = "0x001";
    /*
        FIT SWT specific, marks AND frame in RMW bits operation */
    static constexpr const char* _RMW_BITS_AND_PREFIX_ = "0x002";
    /*
        FIT SWT specific, marks OR frame in RMW bits operation  */
    static constexpr const char* _RMW_BITS_OR_PREFIX_ = "0x003";
    /*
        FIT SWT specific, marks RMW sum operation   */
    static constexpr const char* _RMW_SUM_PREFIX_ = "0x004";

    private:

    /*
        Stors sequence in the ALF-ready format*/
    std::string m_buffer;

    /*
        Buffer for RMW bits mask. Used by passMask  */
    uint32_t m_mask[2];
    
};
}// fit_swt


