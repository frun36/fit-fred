#include<cstdint>
#include<list>
#include<algorithm>
#include<optional>
#include<cstring>


class AlfResponseParser
{
    public:

    struct SwtFrame
    {
        SwtFrame() = default;
        SwtFrame(const char* src);

        uint16_t prefix;
        uint32_t address;
        uint32_t data;
    };
    
    struct Line
    {
        Line() = default;
        Line(const char* hex, int64_t len);

        enum class Type {ResponseToRead, ResponseToWrite} type;
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

        explicit iterator(const char* sequence);

        private:

        int64_t getLineLen() const;
        
        const char* m_sequence;
        std::optional<Line> m_currentLine;
    };
    
    AlfResponseParser(const char* response): m_sequence(response), m_sequenceLen(strlen(response)) {}

    iterator begin();
    iterator end();

    bool isSuccess() const;

    static constexpr const int64_t _SWT_LEN_ = 21;

    private:

    const char* m_sequence;
    int64_t m_sequenceLen;

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
    }
}

inline uint8_t stringToByte(char c1, char c2)
{
    return 16 * charToHex(c1) + charToHex(c2);
}