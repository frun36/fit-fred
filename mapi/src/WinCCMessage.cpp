#include "WinCCMessage.h"

explicit WinCCMessage::Request::Request(const std::string& line) {
    std::vector<std::string> arguments = Utility::splitString(line, ",");

    if (arguments.size() < 2)
        throw std::runtime_error("Too few arguments");
    
    name = std::move(arguments[0]);

    if (arguments[1] == "READ") {
        operation = Operation::Read;
        value = std::nullopt;
    } else if (arguments[1] == "WRITE") {
        operation = Operation::Write;
        
        if (arguments.size() < 3)
            throw std::runtime_error(name + ": Too few arguments for WRITE operation");
        
        value = stringToDouble(arguments[2]);

        if (!value.has_value())
            throw std::runtime_error(name + ": Invalid WRITE argument format \"" + arguments[2] + "\"");
    } else {
        throw std::runtime_error(name + ": Invalid operation \"" + arguments[1] + "\"");
    }
}

explicit WinCCMessage::WinCCMessage(const std::string& input) {
    std::vector<std::string> lines = Utility::splitString(input, "\n"); // CRLF for Windows-based WinCC?

    for (const auto& line: lines)
        m_requests.push_back(Request(line));
}

std::optional<double> WinCCMessage::stringToDouble(std::string str) {
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
        return std::nullopt;

    return value;
}