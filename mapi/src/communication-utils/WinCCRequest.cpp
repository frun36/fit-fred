#include "communication-utils/WinCCRequest.h"
#include <cstdint>
#include <sstream>

WinCCRequest::Command::Command(const std::string& line)
{
    std::vector<std::string> arguments = Utility::splitString(line, ",");

    if (arguments.size() < 2) {
        throw std::runtime_error("Too few arguments");
    }

    name = std::move(arguments[0]);

    if (arguments[1] == "READ") {
        operation = Operation::Read;
        value = std::nullopt;
    } else if (arguments[1] == "WRITE") {
        operation = Operation::Write;

        if (arguments.size() < 3) {
            throw std::runtime_error(name + ": Too few arguments for WRITE operation");
        }

        value = stringToDouble(arguments[2]);

        if (!value.has_value()) {
            throw std::runtime_error(name + ": Invalid WRITE argument format \"" + arguments[2] + "\"");
        }
    } else if (arguments[1] == "WRITE_ELECTRONIC") {
        operation = Operation::WriteElectronic;

        if (arguments.size() < 3) {
            throw std::runtime_error(name + ": Too few arguments for WRITE_ELECTRONIC operation");
        }

        value = stringToDouble(arguments[2]);

        if (!value.has_value()) {
            throw std::runtime_error(name + ": Invalid WRITE_ELECTRONIC argument format \"" + arguments[2] + "\"");
        }
    } else {
        throw std::runtime_error(name + ": Invalid operation \"" + arguments[1] + "\"");
    }
}

WinCCRequest::WinCCRequest(const std::string& input)
{
    std::vector<std::string> lines = Utility::splitString(input, "\n"); // CRLF for Windows-based WinCC?

    for (const auto& line : lines) {
        Command cmd(line);

        if (!m_isWrite.has_value()) {
            m_isWrite = cmd.isWrite();
        } else if (m_isWrite.value() != cmd.isWrite()) {
            throw std::runtime_error(cmd.name + ": attempted operation mixing in single request");
        }

        m_commands.push_back(cmd);
    }
}

std::optional<double> WinCCRequest::stringToDouble(std::string str)
{
    // stringstream is slow - makes for easy code though, I suggest we stick with it for now to avoid premature optimisation
    std::stringstream ss(str);
    double value;
    uint32_t hexTmp;

    if (str.rfind("0x", 0) == 0) {
        ss >> std::hex >> hexTmp;
        value = static_cast<double>(hexTmp);
    } else {
        ss >> value;
    }

    if (ss.fail()) {
        return std::nullopt;
    }

    return value;
}
