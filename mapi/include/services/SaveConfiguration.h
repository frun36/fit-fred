#pragma once
#include "Fred/Mapi/indefinitemapi.h"
#include "Board.h"
#include "database/DatabaseTables.h"
#include <unordered_set>

class SaveConfiguration: public IndefiniteMapi
{
    public:
    /*  Expected line: [CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[VALUE]
    */
    void processExecution() override;

    private:
    std::string makeEntry(std::string_view line);
    void fetchAllConfigs();

    std::unordered_map<std::string, std::shared_ptr<Board>> m_Boards;
    std::unordered_set<std::string> m_knownConfigs;
};