#pragma once

#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <sstream>

#include "Parser/utility.h"

class WinCCMessage {
public:
    struct Exception : public std::runtime_error {
        Exception(std::string msg) : std::runtime_error(msg) {}
    };

    struct Request {
        enum class Operation { Read, Write };
        
        std::string name;
        Operation operation;
        std::optional<double> value;

        explicit Request(const std::string& line);
    };

private:
    std::vector<Request> m_requests;

public:
    explicit WinCCMessage(const std::string& input);

    const std::vector<Request>& getRequests() const {
        return m_requests;
    }

    static double stringToDouble(std::string str);
};