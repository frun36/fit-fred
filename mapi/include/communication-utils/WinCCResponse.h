#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>

class WinCCResponse
{
   private:
    std::string m_contents;

   public:
    WinCCResponse& addParameter(const std::string& name, const std::vector<double>& data)
    {
        std::stringstream ss;
        ss << name;

        for (const auto& val : data) {
            if (trunc(val) == val) {
                ss << "," << static_cast<int64_t>(val);
            } else {
                ss << "," << std::fixed << std::setprecision(3) << val;
            }
        }

        ss << "\n";

        m_contents += ss.str();

        return *this;
    }

    const std::string& getContents() const
    {
        return m_contents;
    }
};