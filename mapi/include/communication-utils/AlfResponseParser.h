#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <string_view>

class AlfResponseParser
{
   public:
    struct SwtFrame {
        SwtFrame() = default;
        SwtFrame(std::string_view src);

        uint16_t prefix;
        uint32_t address;
        uint32_t data;
    };

    struct Line {
        Line() = default;
        Line(std::string_view hex, int64_t len);

        enum class Type { ResponseToRead,
                          ResponseToWrite } type;
        SwtFrame frame;
        int64_t length;
    };

    class iterator
    {
       public:
        bool operator!=(const iterator& itr);
        iterator& operator++();
        // iterator operator++(int) const;
        Line operator*() const;

        explicit iterator(std::string_view sequence);
        ~iterator() { delete m_currentLine; }

       private:
        int64_t getLineLen() const;

        std::string_view m_sequence;
        Line* m_currentLine = nullptr; // proved faster than std::optional or std::unique_ptr
    };

    AlfResponseParser(std::string_view response) : m_sequence(response) {}

    iterator begin();
    iterator end();

    bool isSuccess() const;

    static constexpr const int64_t _SWT_LEN_ = 21;

   private:
    std::string_view m_sequence;
};

inline uint8_t charToHex(char ch)
{
    switch (ch) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'A':
        case 'a':
            return 10;
        case 'B':
        case 'b':
            return 11;
        case 'C':
        case 'c':
            return 12;
        case 'D':
        case 'd':
            return 13;
        case 'E':
        case 'e':
            return 14;
        case 'F':
        case 'f':
            return 15;
        default:
            throw std::invalid_argument("Invalid hexadecimal character");
    }
}

inline uint8_t stringToByte(char c1, char c2)
{
    return static_cast<uint8_t>((charToHex(c1) << 4) | charToHex(c2));
}

inline uint8_t stringToByte(const char* c)
{
    return (charToHex(c[0]) << 4) | charToHex(c[1]);
}
