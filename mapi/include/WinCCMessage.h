#pragma once

#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <sstream>

#include "Parser/utility.h"

class WinCCMessage {
public:
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

    static std::optional<double> stringToDouble(std::string str);
};