#pragma once
#include <cmath>
#include <memory>
#include <unordered_map>
#include "database/DatabaseViews.h"
#include "Board.h"

typedef uint8_t columnIdx;

class FitData
{
   public:
    FitData();
    bool isReady() const { return m_ready; }

    static constexpr uint32_t BaseAddressPMA = 0x0200;
    static constexpr uint32_t BaseAddressPMC = 0x1600;
    static constexpr uint32_t AddressSpaceSizePM = 0x0200;

    std::unordered_map<std::string, std::shared_ptr<Board>>& getBoards() { return m_boards; }
    std::unordered_map<std::string, std::list<std::string>>& getStatusList() { return m_statusParameters; }

    struct PmHistogramBlock {
        uint32_t baseAddress;
        uint32_t regBlockSize;
        int32_t startBin;
        uint32_t binsPerRegister;
        enum class Direction { Positive,
                               Negative } direction;
    };

    struct PmHistogram {
        std::string name;
        std::optional<PmHistogramBlock> positiveBins;
        std::optional<PmHistogramBlock> negativeBins;
    };

   std::unordered_map<std::string,PmHistogram>& getPmHistograms() {return m_PmHistograms;}

   private:
    struct DeviceInfo {
        DeviceInfo(std::vector<MultiBase*>&);

        std::string name;
        enum class Side { A,
                          C } side;
        enum class BoardType { PM,
                               TCM } type;
        uint32_t index;
        bool isConnected;
    };

    [[nodiscard]] bool fetchBoardParamters(std::string boardType);
    [[nodiscard]] bool fetchEnvironment();
    [[nodiscard]] bool fetchConnectedDevices();
    [[nodiscard]] bool fetchPmHistogramStructure();

    bool m_ready;
    std::shared_ptr<Board> parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable);
    Board::ParameterInfo parseParameter(std::vector<MultiBase*>&);
    void parseEnvVariables(std::vector<std::vector<MultiBase*>>& settingsTable);

    std::list<std::string> constructStatusParametersList(std::string_view boardName);
    std::shared_ptr<Board> constructBoardFromTemplate(std::string name, uint32_t address, bool isConnected, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main = nullptr);

    bool checkEnvironment();

    std::unordered_map<std::string, std::shared_ptr<Board>> m_templateBoards;
    std::unordered_map<std::string, std::list<std::string>> m_statusParameters;
    std::unordered_map<std::string, std::shared_ptr<Board>> m_boards;
    std::unordered_map<std::string, PmHistogram> m_PmHistograms;
    std::shared_ptr<EnvironmentVariables> m_environmentalVariables;
};
