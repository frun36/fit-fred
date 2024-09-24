#pragma once

#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <sstream>

#include "Parser/utility.h"

class WinCCRequest {
public:
    enum class Operation { Read, Write };
    
    struct Command {
        std::string name;
        Operation operation;
        std::optional<double> value; // this assumes only single-valued writes

        Command(const std::string& name, Operation operation, std::optional<double> value) : name(name), operation(operation), value(value) {}
        explicit Command(const std::string& line);

        bool operator==(const Command& other) const {
            return name == other.name && operation == other.operation && value == other.value;
        }
    };

private:
    std::vector<Command> m_commands;
    std::optional<Operation> m_reqType = std::nullopt;

public:
    explicit WinCCRequest(const std::string& input);

    const std::vector<Command>& getCommands() const {
        return m_commands;
    }

    static std::optional<double> stringToDouble(std::string str);
};