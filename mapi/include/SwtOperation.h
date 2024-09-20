#include <cstdint>

#include "Parameter.h"
#include "WinCCRequest.h"

#include <optional>
#include <string>
#include <iomanip>

struct SwtOperation {
    enum class Type {
        Read,
        Write,
        RmwBits,
        RmwSum,
    };

    std::string parameterName;
    Type type;
    uint32_t baseAddress;
    uint32_t data0;
    uint32_t data1;
    uint32_t regblockSize;

    SwtOperation(Type type, uint32_t baseAddress, uint32_t data0 = 0, uint32_t data1 = 0)
        : type(type), baseAddress(baseAddress), data0(data0), data1(data1), regblockSize(1) { }

    SwtOperation(const std::string& parameterName, Type type, uint32_t baseAddress, uint32_t regblockSize, uint32_t data0 = 0, uint32_t data1 = 0)
        : parameterName(parameterName), type(type), baseAddress(baseAddress), data0(data0), data1(data1), regblockSize(regblockSize) { }

    std::string getRequest() const {
        std::stringstream ss;
        for (uint32_t address = baseAddress; address < baseAddress + regblockSize - 1; address++) {
            ss << "0x00";
            switch (type) {
                case Type::Read:
                    ss << "0";
                    break;
                case Type::Write:
                    ss << "1";
                    break;
                case Type::RmwBits:
                    ss << "2";
                    break;
                case Type::RmwSum:
                    ss << "4";
                    break;
                default:
                    ss << "5";
            }

            ss << std::hex << std::setw(8) << std::setfill('0') << baseAddress;
            ss << std::hex << std::setw(8) << std::setfill('0') << data0;
            ss << ",write";
            if (type == Type::Read || type == Type::RmwBits || type == Type::RmwSum) {
                ss << "\nread";
            }

            if (type == Type::RmwBits) {
                ss << "\n0x003";
                ss << std::hex << std::setw(8) << std::setfill('0') << baseAddress;
                ss << std::hex << std::setw(8) << std::setfill('0') << data1;
                ss << ",write";
            }
            ss << "\n";
        }

        return ss.str();
    }

    static SwtOperation fromParameterOperation(const Parameter& parameter, WinCCRequest::Command::Operation operation, std::optional<double> data);
};