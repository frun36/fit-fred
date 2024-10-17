#pragma once

#include <cstdint>
#include <list>
#include <stdexcept>
#include <algorithm>
#include <optional>
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
        iterator operator++(int) const;
        Line operator*() const;

        explicit iterator(std::string_view sequence);

       private:
        int64_t getLineLen() const;

        std::string_view m_sequence;
        std::optional<Line> m_currentLine;
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
    if (ch >= '0' && ch <= '9')
        return static_cast<uint8_t>(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return static_cast<uint8_t>(ch - 'A' + 10);
    else if (ch >= 'a' && ch <= 'f')
        return static_cast<uint8_t>(ch - 'a' + 10);
    else
        throw std::invalid_argument("Invalid hexadecimal character");
}

inline uint8_t stringToByte(char c1, char c2)
{
    return static_cast<uint8_t>((charToHex(c1) << 4) | charToHex(c2));
}
