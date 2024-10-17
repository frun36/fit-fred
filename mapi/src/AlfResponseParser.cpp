#include<cstring>
#include"AlfResponseParser.h"
#include<iostream>

bool AlfResponseParser::isSuccess() const
{
    return ( m_sequence.compare(0, strlen("success") , "success") == 0);
}

AlfResponseParser::SwtFrame::SwtFrame(std::string_view src)
{
    data = (static_cast<uint32_t>(stringToByte(src[11], src[12])) << 24) + (static_cast<uint32_t>(stringToByte(src[13], src[14])) << 16) + (static_cast<uint32_t>(stringToByte(src[15], src[16])) << 8) + static_cast<uint32_t>(stringToByte(src[17], src[18]));
    address = (static_cast<uint32_t>(stringToByte(src[3], src[4])) << 24) + (static_cast<uint32_t>(stringToByte(src[5], src[6])) << 16) + (static_cast<uint32_t>(stringToByte(src[7], src[8])) << 8) + static_cast<uint32_t>(stringToByte(src[9], src[10]));
    prefix = (static_cast<uint16_t>(stringToByte('0', src[0])) << 8) +  static_cast<uint16_t>(stringToByte(src[1], src[2]));
}

AlfResponseParser::Line::Line(std::string_view hex, int64_t len): length(len)
{
    if(len == 1)
    {
        type = Type::ResponseToWrite;
    }
    else if(len == _SWT_LEN_)
    {
        type = Type::ResponseToRead;
        hex = &hex[2];
        frame = SwtFrame(hex);
    }
    else
    {
        throw std::runtime_error("Invalid line: " + std::to_string(len) + hex.data());
    }
}

AlfResponseParser::iterator::iterator(std::string_view sequence): m_sequence(sequence)
{
    if(m_sequence.at(0) == '\0' || m_sequence.at(0) == '\n'){
        m_currentLine = std::nullopt;
    }
    else{
        m_currentLine = Line(m_sequence, getLineLen());
    }
}

int64_t AlfResponseParser::iterator::getLineLen() const
{
    int64_t beg_ptr = 0;
    int64_t end_ptr = 0;

    for(int64_t pos = 0; ; pos++){

        switch(m_sequence[pos])
        {    
            case '\0':
            case '\n':
            {
                end_ptr = pos;
            }
            break;

        default:
            continue;
        break;
        }

        break;
    }

    return (end_ptr - beg_ptr);
}

AlfResponseParser::iterator& AlfResponseParser::iterator::operator++()
{
    if(m_sequence.at(0) == '\0')
    {
        throw std::runtime_error("Iterator points to end(), cannot increment");
    }
    if(m_sequence[m_currentLine->length] != '\0' && m_sequence[m_currentLine->length+1] != '\0'){  
        m_sequence = m_sequence.data() + m_currentLine->length+1;
        m_currentLine = Line(m_sequence, getLineLen());
        return *this;
    }

    m_sequence = ( m_sequence[m_currentLine->length] == '\0')? m_sequence.data() + m_currentLine->length - 1: m_sequence.data() + m_currentLine->length;
    m_currentLine = std::nullopt;
    return *this;
}

AlfResponseParser::iterator AlfResponseParser::iterator::operator++(int) const
{
    AlfResponseParser::iterator tmp(*this);
    tmp++;
    return tmp;
}

AlfResponseParser::Line AlfResponseParser::iterator::operator*() const
{
    if(m_currentLine == std::nullopt){
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
    if(isSuccess()) return iterator(m_sequence.data()+strlen("success\n"));
    else return iterator(m_sequence.data()+strlen("failure\n"));
}

AlfResponseParser::iterator AlfResponseParser::end()
{
    return iterator(m_sequence.data() + m_sequence.size()-1);
}