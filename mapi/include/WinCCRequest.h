#pragma once

#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <sstream>

#include "Parser/utility.h"

class WinCCRequest {
public:
    struct Command {
        enum class Operation { Read, Write };
        
        std::string name;
        Operation operation;
        std::optional<double> value;

        explicit Command(const std::string& line);
    };

private:
    std::vector<Command> m_commands;

public:
    explicit WinCCRequest(const std::string& input);

    const std::vector<Command>& getCommands() const {
        return m_commands;
    }

    static std::optional<double> stringToDouble(std::string str);
};