#include "mapifactory.h"
#include <memory>
#include "FitData.h"
#include "services/histograms/TcmHistograms.h"

MapiFactory::MapiFactory(Fred* fred) : m_fred(fred)
{
    try {
        generateObjects();
    } catch (const exception& e) {
        Print::PrintError(e.what());
        exit(EXIT_FAILURE);
    }
}

void MapiFactory::generateObjects()
{
    FitData boardsData;
    if (boardsData.isReady() == false) {
        Print::PrintError("Board data fetching failed! Aborting");
        exit(-1);
    }
    Print::PrintVerbose("Registering MAPI Objects");

    m_configurationsObject = make_unique<Configurations>(m_fred->Name(), boardsData.getBoards());
    m_fred->registerMapiObject(m_fred->Name() + "/TCM/TCM0/CONFIGURATIONS", m_configurationsObject.get());
    std::shared_ptr<Board> tcm;
    std::vector<std::shared_ptr<Board>> pms;
    for (auto [boardName, board] : boardsData.getBoards()) {
        string section;
        if (board->isConnected() == false) {
            continue;
        }

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
        m_counterRatesObjects.emplace_back(board);

        string servicePrefix = m_fred->Name() + "/" + section + "/" + boardName + "/";

        m_fred->registerMapiObject(servicePrefix + "PARAMETERS", &m_parametersObjects.back());
        m_fred->registerMapiObject(servicePrefix + "STATUS", &m_statusObjects.back());
        m_fred->registerMapiObject(servicePrefix + "_INTERNAL_CONFIGURATIONS", dynamic_cast<Mapi*>(m_configurationsObject->getBoardConfigurationServices().at(boardName).get()));
        m_fred->registerMapiObject(servicePrefix + "RESET", &m_resetObjects.back());
        m_fred->registerMapiObject(servicePrefix + "COUNTER_RATES", &m_counterRatesObjects.back());
        if (!board->isTcm()) {
            m_pmHistogramsObjects.emplace_back(board, boardsData.getPmHistograms());
            m_fred->registerMapiObject(servicePrefix + "HISTOGRAMS", &m_pmHistogramsObjects.back());
        }
        Print::PrintVerbose(boardName + " registered");
    }

    m_resetSystem = std::make_unique<ResetFEE>(tcm, pms);
    m_fred->registerMapiObject(string_utils::concatenate(m_fred->Name(), "/TCM/TCM0/RESET_SYSTEM"), m_resetSystem.get());
    m_resetError = std::make_unique<ResetErrors>(tcm, pms);
    m_fred->registerMapiObject(string_utils::concatenate(m_fred->Name(), "/TCM/TCM0/RESET_ERRORS"), m_resetError.get());
    m_setPhaseDelay = std::make_unique<SetPhaseDelay>(tcm);
    m_fred->registerMapiObject(string_utils::concatenate(m_fred->Name(), "/TCM/TCM0/SET_PHASE_DELAY"), m_setPhaseDelay.get());
    m_saveConfiguration = std::make_unique<ConfigurationDatabaseBroker>(boardsData.getBoards());
    m_fred->registerMapiObject(string_utils::concatenate(m_fred->Name(), "/TCM/TCM0/CONFIGURATION_DB_BROKER"), m_saveConfiguration.get());

    m_tcmHistograms = std::make_unique<TcmHistograms>(tcm);
    m_fred->registerMapiObject(m_fred->Name() + "/TCM/TCM0/HISTOGRAMS", m_tcmHistograms.get());
}
