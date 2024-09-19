#pragma once

#include <string>
#include <vector>

class WinCCResponse {
private:
    std::string m_contents;
public:
    void addParameter(const std::string& name, const std::vector<double>& data) {
        std::string line = name;

        for (const auto& val : data)
            line += "," + std::to_string(val);
        
        line += "\n";

        m_contents += line;
    }

    const std::string& getContents() const {
        return m_contents;
    }
};