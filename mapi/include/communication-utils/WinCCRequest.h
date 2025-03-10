#pragma once

#include <string>
#include <vector>
#include <optional>
#include "utils/utils.h"

#ifdef FIT_UNIT_TEST

#include "../../../test/mocks/include/utility.h"

#else

#include "Parser/utility.h"

#endif

class WinCCRequest
{
   public:
    enum class Operation { Read,
                           Write,
                           WriteElectronic };

    struct Command {
        std::string name;
        Operation operation;
        std::optional<double> value; // this assumes only single-valued writes

        Command(const std::string& name, Operation operation, std::optional<double> value) : name(name), operation(operation), value(value) {}
        explicit Command(const std::string& line);

        bool operator==(const Command& other) const
        {
            return name == other.name && operation == other.operation && value == other.value;
        }

        bool isWrite() const
        {
            return operation == Operation::Write || operation == Operation::WriteElectronic;
        }
    };

   private:
    std::vector<Command> m_commands;
    std::optional<bool> m_isWrite = std::nullopt;

   public:
    explicit WinCCRequest(const std::string& input);

    const std::vector<Command>& getCommands() const
    {
        return m_commands;
    }

    static std::optional<double> stringToDouble(std::string str);

    bool isWrite() const
    {
        return m_isWrite.has_value() && *m_isWrite;
    }

    template <typename T>
    static std::string writeRequest(std::string_view param, T value)
    {
        return string_utils::concatenate(param, ",WRITE,", std::to_string(value));
    }

    template <typename T>
    static std::string writeElectronicRequest(std::string_view param, T value)
    {
        static_assert((!std::is_same<T, double>::value) && (!std::is_same<T, float>::value), "Electronic value has to be integer type");
        return string_utils::concatenate(param, ",WRITE_ELECTRONIC,", std::to_string(value));
    }

    static std::string readRequest(std::string_view param)
    {
        return string_utils::concatenate(param, ",READ");
    }

    static std::string& appendToRequest(std::string& mess, const std::string& newRequest)
    {
        return mess.append(newRequest).append("\n");
    }
};
