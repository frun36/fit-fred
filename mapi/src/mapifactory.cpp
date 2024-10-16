#include "mapifactory.h"
#include "FitData.h"

MapiFactory::MapiFactory(Fred* fred)
{
    m_fred = fred;

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
    }
    Print::PrintVerbose("Registering MAPI Objects");

    m_configurationsObject = Configurations(m_fred, boardsData.getBoards());
    m_fred->registerMapiObject(m_fred->Name() + "/TCM/TCM0/CONFIGURATIONS", &m_configurationsObject);

    for (auto [boardName, board] : boardsData.getBoards()) {
        string section;
        if (boardName.find("TCM") != std::string::npos) {
            section = "TCM";
        } else {
            section = "PM";
        }
        m_parametersObjects.emplace_back(board);

        m_statusObjects.emplace_back(board, boardsData.getStatusList().at(section));            
        string servicePrefix = m_fred->Name() + "/" + section + "/" + boardName + "/";

        m_fred->registerMapiObject(servicePrefix + "PARAMETERS", &m_parametersObjects.back());
        m_fred->registerMapiObject(servicePrefix + "STATUS", &m_statusObjects.back());
        m_fred->registerMapiObject(servicePrefix + "_INTERNAL_CONFIGURATIONS", dynamic_cast<Mapi*>(m_configurationsObject.getBoardConfigurationServices().at(boardName).get()));
        
        Print::PrintVerbose(boardName + " registered");
    }
}
