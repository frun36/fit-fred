#include "mapifactory.h"
#include "FitData.h"

MapiFactory::MapiFactory(Fred* fred) : m_fred(fred)
{
    try {
        generateObjects();
    } catch (const exception& e) {
        Print::PrintError(e.what());
        exit(EXIT_FAILURE);
    }
}

MapiFactory::~MapiFactory()
{
    for (size_t i = 0; i < m_mapiObjects.size(); i++) {
        delete m_mapiObjects[i];
    }
}

void MapiFactory::generateObjects()
{
    FitData boardsData;
    if (boardsData.isReady() == false) {
        Print::PrintError("Configuration failed! Aborting");
        exit(-1);
    }
    Print::PrintVerbose("Registering MAPI Objects");

    m_configurationsObject = Configurations(m_fred->Name(), boardsData.getBoards());
    m_fred->registerMapiObject(m_fred->Name() + "/TCM/TCM0/CONFIGURATIONS", &m_configurationsObject);
    std::shared_ptr<Board> tcm;
    std::vector<std::shared_ptr<Board>> pms;
    for (auto [boardName, board] : boardsData.getBoards()) {
        string section;
        if (boardName.find("TCM") != std::string::npos) {
            section = "TCM";
            tcm = board;
        } else {
            section = "PM";
            pms.emplace_back(board);
        }
        m_parametersObjects.emplace_back(board);
        m_statusObjects.emplace_back(board, boardsData.getStatusList().at(section));
        m_resetObjects.emplace_back(board);
        string servicePrefix = m_fred->Name() + "/" + section + "/" + boardName + "/";

        m_fred->registerMapiObject(servicePrefix + "PARAMETERS", &m_parametersObjects.back());
        m_fred->registerMapiObject(servicePrefix + "STATUS", &m_statusObjects.back());
        m_fred->registerMapiObject(servicePrefix + "_INTERNAL_CONFIGURATIONS", dynamic_cast<Mapi*>(m_configurationsObject.getBoardConfigurationServices().at(boardName).get()));
        m_fred->registerMapiObject(servicePrefix + "RESET", dynamic_cast<Mapi*>(&m_resetObjects.back()));

        Print::PrintVerbose(boardName + " registered");
    }

    m_resetSystem = std::make_shared<ResetFEE>(tcm, pms);
    m_fred->registerMapiObject(string_utils::concatenate(m_fred->Name(), "/TCM/TCM0/RESET_SYSTEM"), dynamic_cast<Mapi*>(m_resetSystem.get()));
    m_resetError = std::make_shared<ResetErrors>(tcm, pms);
    m_fred->registerMapiObject(string_utils::concatenate(m_fred->Name(), "/TCM/TCM0/RESET_ERRORS"), dynamic_cast<Mapi*>(m_resetError.get()));
}
