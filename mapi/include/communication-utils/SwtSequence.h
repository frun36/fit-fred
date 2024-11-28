#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <array>
/*
    Creates SWT sequence in the ALF-ready format */
class SwtSequence
{
   public:
    /*
        SWT operation type  */
    enum class Operation { Read,
                           Write,
                           RMWbits,
                           RMWsum };

    struct SwtOperation {
        SwtOperation(Operation type, uint32_t address, std::array<uint32_t, 2> data = std::array<uint32_t, 2>(), bool expectResponse = false);
        Operation type;
        uint32_t address;
        std::array<uint32_t, 2> data;
        bool expectResponse;
    };

    SwtSequence() : m_buffer(_SEQUENCE_PREFIX_) {}
    SwtSequence(const std::vector<SwtOperation>& operations);

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
    SwtSequence& addOperation(Operation type, uint32_t address, const uint32_t* data = nullptr, bool expectResponse = true);
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
    SwtSequence& addOperation(Operation type, const char* address, const uint32_t* data = nullptr, bool expectResponse = true);

    SwtSequence& addOperation(SwtOperation&& operation);
    SwtSequence& addOperation(const SwtOperation& operation);

    /*
        Returns stored sequence */
    const std::string& getSequence() const { return m_buffer; }

    /*
        Creates mask for RMW bits operation */
    static void createMask(uint32_t firstBit, uint32_t lastBit, uint32_t value, uint32_t* data);
    static uint32_t createANDMask(uint32_t firstBit, uint32_t bitLength);

    /*
        Creates mask in an internal buffer and returns pointer to it. */
    const uint32_t* passMasks(uint32_t firstBit, uint32_t lastBit, uint32_t value);

    /*
        Translate word to hex format */
    static std::string wordToHex(uint32_t word);

    /*
        Translate half byte to hex format. Inline.*/
    static char halfByteToHex(uint8_t halfByte)
    {
        static constexpr char hexDigits[] = "0123456789ABCDEF";
        return hexDigits[halfByte & 0x0F];
    }

    /*
        Prefix addded at the begging of every sequence  */
    static constexpr const char* _SEQUENCE_PREFIX_ = "sc_reset";
    /*
        CRU special word, appended to every SWT frame   */
    static constexpr const char* _FRAME_POSTFIX_ = ",write";
    /*
        CRU special word, appended to sequence if response to current operation (one SWT frame) is expected */
    static constexpr const char* _READ_WORD_ = "\nread";

    /*
        FIT SWT specific, marks read operation  */
    static constexpr const char* _READ_PREFIX_ = "\n0x000";
    /*
        FIT SWT specific, marks write operation */
    static constexpr const char* _WRITE_PREFIX_ = "\n0x001";
    /*
        FIT SWT specific, marks AND frame in RMW bits operation */
    static constexpr const char* _RMW_BITS_AND_PREFIX_ = "\n0x002";
    /*
        FIT SWT specific, marks OR frame in RMW bits operation  */
    static constexpr const char* _RMW_BITS_OR_PREFIX_ = "\n0x003";
    /*
        FIT SWT specific, marks RMW sum operation   */
    static constexpr const char* _RMW_SUM_PREFIX_ = "\n0x004";

   private:
    /*
        Stores sequence in the ALF-ready format*/
    std::string m_buffer;

    /*
        Buffer for RMW bits mask. Used by passMasks  */
    std::array<uint32_t, 2> m_mask;
};
