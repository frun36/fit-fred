#include<cmath>
#include<memory>
#include<unordered_map>

#include"Database/databaseinterface.h"
#include"Board.h"

typedef uint8_t columnIdx;

class FitData
{
    public:
    FitData();
    bool isReady() const { return m_ready; }

    static constexpr uint32_t BaseAddressPMA = 0x0200;
    static constexpr uint32_t BaseAddressPMC = 0x1600;
    static constexpr uint32_t AddressSpaceSizePM = 0x0200;

    std::unordered_map<std::string, std::shared_ptr<Board>> getBoards() {return m_boards;}

    private:
    bool m_ready;
    std::shared_ptr<Board> parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable);
    std::shared_ptr<Board> constructBoardFromTemplate(std::string name, uint32_t address, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main=nullptr);

    std::unordered_map<std::string, std::shared_ptr<Board>> m_templateBoards;
    std::unordered_map<std::string, std::shared_ptr<Board>> m_boards;
};


class ParametersTable
{
public: 
    struct Parameter
    {
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

        static constexpr const char* TypePM = "PM";
        static constexpr const char* TypeTCM = "TCM";
        static constexpr const char* TypeHistogram = "PM_HIST";

        static constexpr const char* RefreshSYNC = "SYNC";
        static constexpr const char* RefreshCNT = "CNT";

        static constexpr const char* ExprTRUE = "Y";
        static constexpr const char* ExprFALSE = "N";

        static Board::ParameterInfo::Equation parseEquation(std::string eq);
        static Board::ParameterInfo buildParameter(std::vector<MultiBase*>&);    
    };
 
    static bool parseBoolean(MultiBase* field);
    static uint32_t parseHex(MultiBase* field);
};


class ConnectedDevicesTable
{
    public:

    struct Device
    {
        Device(std::vector<MultiBase*>&);

        std::string name;
        enum class Side{A,C} side;
        enum class BoardType{PM, TCM} type;
        uint32_t index;

        static constexpr columnIdx Name = 0;
        static constexpr columnIdx Type = 1;

        static constexpr const char* TypePM = "PM";
        static constexpr const char* TypeTCM = "TCM";
    };
};