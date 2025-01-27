#include "communication-utils/AlfResponseParser.h"
#include <cstring>
#include <iostream>

static constexpr auto SUCCESS_STR = "success\n";
static constexpr auto FAILURE_STR = "failure\n";
static constexpr auto SUCCESS_STR_LEN = sizeof(SUCCESS_STR); // or sizeof("success") - 1
static constexpr auto FAILURE_STR_LEN = sizeof(FAILURE_STR); // or sizeof("success") - 1

bool AlfResponseParser::isSuccess() const
{
    return (m_sequence.compare(0, SUCCESS_STR_LEN, SUCCESS_STR) == 0);
}

AlfResponseParser::SwtFrame::SwtFrame(std::string_view src)
{
    const char* s = src.data(); // faster that indexing into string_view (no check)
    data = (static_cast<uint32_t>(stringToByte(s + 11)) << 24) + (static_cast<uint32_t>(stringToByte(s + 13)) << 16) + (static_cast<uint32_t>(stringToByte(s + 15)) << 8) + static_cast<uint32_t>(stringToByte(s + 17));
    address = (static_cast<uint32_t>(stringToByte(s + 3)) << 24) + (static_cast<uint32_t>(stringToByte(s + 5)) << 16) + (static_cast<uint32_t>(stringToByte(s + 7)) << 8) + static_cast<uint32_t>(stringToByte(s + 9));
    prefix = (static_cast<uint16_t>(stringToByte('0', s[0])) << 8) + static_cast<uint16_t>(stringToByte(s + 1));
}

AlfResponseParser::Line::Line(std::string_view hex, int64_t len) : length(len)
{
    if (len == 1) {
        type = Type::ResponseToWrite;
    } else if (len == _SWT_LEN_) {
        type = Type::ResponseToRead;
        hex.remove_prefix(2);
        frame = SwtFrame(hex);
    } else {
        throw std::runtime_error("Invalid line: " + std::to_string(len) + hex.data());
    }
}

AlfResponseParser::iterator::iterator(std::string_view sequence) : m_sequence(sequence)
{
    if (m_sequence.at(0) == '\0' || m_sequence.at(0) == '\n') {
        m_currentLine = std::nullopt;
    } else {
        m_currentLine = Line(m_sequence, getLineLen());
    }
}

int64_t AlfResponseParser::iterator::getLineLen() const
{
    const char* beg = m_sequence.begin();
    const char* end = m_sequence.begin();

    for (;; end++) {
        if (*end == '\0' || *end == '\n') {
            return (end - beg);
        }
    }
}

AlfResponseParser::iterator& AlfResponseParser::iterator::operator++()
{
    if (m_sequence.at(0) == '\0') {
        throw std::runtime_error("Iterator points to end(), cannot increment");
    }
    if (m_sequence[m_currentLine->length] != '\0' && m_sequence[m_currentLine->length + 1] != '\0') {
        m_sequence.remove_prefix(m_currentLine->length + 1);
        m_currentLine = Line(m_sequence, getLineLen());
        return *this;
    }

    uint32_t shift = (m_sequence[m_currentLine->length] == '\0') ? m_currentLine->length - 1 : m_currentLine->length;
    m_sequence.remove_prefix(shift);
    m_currentLine = std::nullopt;
    return *this;
}

// AlfResponseParser::iterator AlfResponseParser::iterator::operator++(int) const
// {
//     AlfResponseParser::iterator tmp(*this);
//     tmp++;
//     return std::move(tmp);
// }

AlfResponseParser::Line AlfResponseParser::iterator::operator*() const
{
    if (m_currentLine == std::nullopt) {
        throw std::runtime_error("Iterator points to end(), cannot dereference");
    }
    return *m_currentLine;
}

bool AlfResponseParser::iterator::operator!=(const iterator& itr)
{
    return itr.m_sequence.data() != m_sequence.data();
}

AlfResponseParser::iterator AlfResponseParser::begin()
{
    if (m_sequence == SUCCESS_STR || m_sequence == FAILURE_STR) {
        return end();
    }
    if (isSuccess())
        return iterator(m_sequence.substr(SUCCESS_STR_LEN));
    else
        return iterator(m_sequence.substr(FAILURE_STR_LEN));
}

AlfResponseParser::iterator AlfResponseParser::end()
{
    return iterator(m_sequence.data() + m_sequence.size() - 1);
}