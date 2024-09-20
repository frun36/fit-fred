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

    Type type;
    uint32_t address;
    uint32_t data0;
    uint32_t data1;

    SwtOperation(Type type, uint32_t baseAddress, uint32_t data0 = 0, uint32_t data1 = 0)
        : type(type), address(baseAddress), data0(data0), data1(data1) { }


    std::string getRequest() const {
        std::stringstream ss;
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

        ss << std::hex << std::setw(8) << std::setfill('0') << address;
        ss << std::hex << std::setw(8) << std::setfill('0') << data0;
        ss << ",write";
        if (type == Type::Read || type == Type::RmwBits || type == Type::RmwSum) {
            ss << "\nread";
        }

        if (type == Type::RmwBits) {
            ss << "\n0x003";
            ss << std::hex << std::setw(8) << std::setfill('0') << address;
            ss << std::hex << std::setw(8) << std::setfill('0') << data1;
            ss << ",write";
        }

        if (type != Type::Read) {
            ss << "\n0x000" << std::hex << std::setw(8) << std::setfill('0') << address << "00000000,write\nread";
        }
        ss << "\n";

        return ss.str();
    }

    static SwtOperation fromParameterOperation(const Parameter& parameter, WinCCRequest::Command::Operation operation, std::optional<double> data);
};