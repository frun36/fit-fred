#include<string_view>
#include<cstdint>
#include"Board.h"
#include "Database/databaseinterface.h"
#include"utils.h"
typedef uint8_t columnIdx;

namespace db_utils
{
Equation parseEquation(std::string eq);
}

class ParametersTable
{
   public:
    struct Parameter {
        static constexpr columnIdx Name = 1;
        static constexpr columnIdx BaseAddress = 2;
        static constexpr columnIdx StartBit = 3;
        static constexpr columnIdx EndBit = 4;
        static constexpr columnIdx RegBlockSize = 5;
        static constexpr columnIdx MinValue = 6;
        static constexpr columnIdx MaxValue = 7;
        static constexpr columnIdx IsSigned = 8;
        static constexpr columnIdx IsFifo = 9;
        static constexpr columnIdx IsReadOnly = 10;
        static constexpr columnIdx EqElectronicToPhysic = 11;
        static constexpr columnIdx EqPhysicToElectronic = 12;
        static constexpr columnIdx RefreshType = 13;

        static constexpr std::string_view TypePM{ "PM" };
        static constexpr std::string_view TypeTCM{ "TCM" };
        static constexpr std::string_view TypeHistogram{ "PM_HIST" };

        static constexpr std::string_view RefreshSYNC{ "SYNC" };
        static constexpr std::string_view RefreshCNT{ "CNT" };

        static constexpr std::string_view ExprTRUE{ "Y" };
        static constexpr std::string_view ExprFALSE{ "N" };

        static Board::ParameterInfo buildParameter(std::vector<MultiBase*>&);
    };

    static bool parseBoolean(MultiBase* field);
    static uint32_t parseHex(MultiBase* field);
};

class ConnectedDevicesTable
{
   public:
    struct Device {
        Device(std::vector<MultiBase*>&);

        std::string name;
        enum class Side { A,
                          C } side;
        enum class BoardType { PM,
                               TCM } type;
        uint32_t index;

        static constexpr columnIdx Name = 0;
        static constexpr columnIdx Type = 1;

        static constexpr std::string_view TypePM{ "PM" };
        static constexpr std::string_view TypeTCM{ "TCM" };
    };
};

class SettingsTable
{
   public:
    struct Variable {
        static constexpr columnIdx Name = 0;
        static constexpr columnIdx Equation = 1;
    };
};