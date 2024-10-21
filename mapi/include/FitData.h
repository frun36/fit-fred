#pragma once
#include <cmath>
#include <memory>
#include <unordered_map>
#include "DatabaseUtils.h"
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

   private:
    bool m_ready;
    std::shared_ptr<Board> parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable);
    std::list<std::string> constructStatusParametersList(std::string_view boardName);
    std::shared_ptr<Board> constructBoardFromTemplate(std::string name, uint32_t address, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main = nullptr);
    void parseSettings(std::vector<std::vector<MultiBase*>>& settingsTable);
    bool checkSettings();

    std::unordered_map<std::string, std::shared_ptr<Board>> m_templateBoards;
    std::unordered_map<std::string, std::list<std::string>> m_statusParameters;
    std::unordered_map<std::string, std::shared_ptr<Board>> m_boards;
    std::shared_ptr<EnvironmentFEE> m_settings;
};
