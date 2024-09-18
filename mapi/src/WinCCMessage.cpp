#include "WinCCMessage.h"

explicit WinCCMessage::Request::Request(const std::string& line) {
    std::vector<std::string> arguments = Utility::splitString(line, ",");

    if (arguments.size() < 2)
        throw WinCCMessage::Exception("Too few arguments");
    
    name = arguments[0];

    if (arguments[1] == "READ") {
        operation = Operation::Read;
        value = {};
    } else if (arguments[1] == "WRITE") {
        operation = Operation::Write;
        
        if (arguments.size() < 3)
            throw WinCCMessage::Exception("Too few arguments for WRITE operation");
        
        value = stringToDouble(arguments[2]);
    } else {
        throw WinCCMessage::Exception("Invalid operation \"" + arguments[1] + "\"");
    }
}

explicit WinCCMessage::WinCCMessage(const std::string& input) {
    std::vector<std::string> lines = Utility::splitString(input, "\n"); // CRLF for Windows-based WinCC?

    for (const auto& line: lines)
        m_requests.push_back(Request(line));
}

double WinCCMessage::stringToDouble(std::string str) {
    // stringstream is slow - makes for easy code though, I suggest we stick with it for now to avoid premature optimisation
    std::stringstream ss;
    double value;

    if (str.rfind("0x", 0) == 0) {
        ss << std::hex << str; 
    } else {
        ss << std::dec << str;
    }
    ss >> value;

    if(ss.fail())
        throw WinCCMessage::Exception("Invalid number format \"" + str + "\"");

    return value;
}