#include <string>
#include "FitData.h"
#include "Alfred/print.h"
#include "utils.h"
#include <sstream>
#include <iomanip>

FitData::FitData() : m_ready(false)
{
    Print::PrintInfo("Fetching configuration from data base - start");
    if (DatabaseInterface::isConnected() == false) {
        Print::PrintError("DB connection failed");
        return;
    }

    Print::PrintInfo("Fetching TCM register map");
    auto parametersTCM = DatabaseInterface::executeQuery(ParametersTable::selectBoardParameters(BoardTypesTable::TypeTCM));
    Print::PrintInfo("Fetched " + std::to_string(parametersTCM.size()) + " rows");
    if (parametersTCM.size() == 0) {
        Print::PrintError("TCM register data have not been found!");
        return;
    }
    m_templateBoards.emplace(BoardTypesTable::TypeTCM, parseTemplateBoard(parametersTCM));
    m_statusParameters.emplace(BoardTypesTable::TypeTCM, constructStatusParametersList(BoardTypesTable::TypeTCM));

    Print::PrintInfo("Fetching PM register map");
    auto parametersPM = DatabaseInterface::executeQuery(ParametersTable::selectBoardParameters(BoardTypesTable::TypePM));
    Print::PrintInfo("Fetched " + std::to_string(parametersPM.size()) + " rows");
    if (parametersPM.size() == 0) {
        Print::PrintError("PM register data have not been found!");
        return;
    }
    m_templateBoards.emplace(BoardTypesTable::TypePM, parseTemplateBoard(parametersPM));
    m_statusParameters.emplace(BoardTypesTable::TypePM, constructStatusParametersList(BoardTypesTable::TypePM));

    Print::PrintInfo("Fetching information about connected devices");
    auto connectedDevices = DatabaseInterface::executeQuery(db_utils::selectQuery("CONNECTED_DEVICES", { "*" }));

    Print::PrintInfo("Fetched " + std::to_string(connectedDevices.size()) + " rows");
    if (connectedDevices.size() == 0) {
        Print::PrintError("Lacking data about connected devices");
        return;
    }

    Print::PrintInfo("Fetching information about unit defintion and others settings");
    auto settings = DatabaseInterface::executeQuery(db_utils::selectQuery("FEE_SETTINGS", { "*" }));
    Print::PrintInfo("Fetched " + std::to_string(settings.size()) + " rows");
    parseSettings(settings);
    if (!checkSettings()) {
        return;
    }

    std::shared_ptr<Board> TCM{ nullptr };

    for (auto& deviceRow : connectedDevices) {
        ConnectedDevicesTable::Device device(deviceRow);
        Print::PrintInfo("Registering " + device.name);

        switch (device.type) {
            case ConnectedDevicesTable::Device::BoardType::PM: {
                if (TCM.get() == nullptr) {
                    Print::PrintVerbose("PM row occured before TCM row!");
                    return;
                }

                switch (device.side) {
                    case ConnectedDevicesTable::Device::Side::A:
                        m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index * AddressSpaceSizePM + BaseAddressPMA, m_templateBoards["PM"], TCM));
                        break;

                    case ConnectedDevicesTable::Device::Side::C:
                        m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index * AddressSpaceSizePM + BaseAddressPMC, m_templateBoards["PM"], TCM));
                        break;
                }

            } break;

            case ConnectedDevicesTable::Device::BoardType::TCM: {
                TCM = m_boards.emplace(device.name, constructBoardFromTemplate(device.name, 0x0, m_templateBoards["TCM"])).first->second;
            } break;
        }
    }

    m_ready = true;
}

std::shared_ptr<Board> FitData::parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable)
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TemplateBoard", 0x0);
    for (auto& row : boardTable) {
        Print::PrintVerbose("Parsing parameter: " + row[ParametersTable::Parameter::Name]->getString());
        board->emplace(ParametersTable::Parameter::buildParameter(row));
    }
    Print::PrintVerbose("Board parsed successfully");
    return board;
}

std::shared_ptr<Board> FitData::constructBoardFromTemplate(std::string name, uint32_t address, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main)
{
    std::shared_ptr<Board> board = std::make_shared<Board>(name, address, main, m_settings);
    for (const auto& parameter : templateBoard->getParameters()) {
        board->emplace(parameter.second);
    }

    return board;
}

void FitData::parseSettings(std::vector<std::vector<MultiBase*>>& settingsTable)
{
    m_settings = std::make_shared<EnvironmentFEE>();
    for (auto& row : settingsTable) {
        std::string name = row[SettingsTable::Variable::Name]->getString();
        Equation equation = db_utils::parseEquation(row[SettingsTable::Variable::Equation]->getString());
        Print::PrintVerbose("Parsing " + name);
        Print::PrintVerbose("Equation: " + equation.equation);
        m_settings->emplace(
            EnvironmentFEE::Variable(name, equation));
        if (equation.variables.empty()) {
            m_settings->updateVariable(name);
        }
    }
    for (auto& row : settingsTable) {
        std::string name = row[SettingsTable::Variable::Name]->getString();
        Print::PrintVerbose("Updating " + name);
        m_settings->updateVariable(name);
        Print::PrintVerbose("Updated " + name + " to " + std::to_string(m_settings->getVariable(name)));
    }
}

bool FitData::checkSettings()
{
    if (!m_settings->doesExist(environment::parameters::ExtenalClock.data())) {
        Print::PrintError(std::string(environment::parameters::ExtenalClock.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::InternalClock.data())) {
        Print::PrintError(std::string(environment::parameters::InternalClock.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::SystemClock.data())) {
        Print::PrintError(std::string(environment::parameters::SystemClock.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::TDC.data())) {
        Print::PrintError(std::string(environment::parameters::TDC.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::PmA0BoardId.data())) {
        Print::PrintError(std::string(environment::parameters::PmA0BoardId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::PmC0BoardId.data())) {
        Print::PrintError(std::string(environment::parameters::PmC0BoardId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::TcmBoardId.data())) {
        Print::PrintError(std::string(environment::parameters::TcmBoardId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::SystemId.data())) {
        Print::PrintError(std::string(environment::parameters::SystemId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::BcIdOffsetDefault.data())) {
        Print::PrintError(std::string(environment::parameters::BcIdOffsetDefault.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::Trigger1Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger1Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::Trigger2Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger2Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }
    if (!m_settings->doesExist(environment::parameters::Trigger3Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger3Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }
    if (!m_settings->doesExist(environment::parameters::Trigger4Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger4Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_settings->doesExist(environment::parameters::Trigger5Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger5Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    return true;
}

std::list<std::string> FitData::constructStatusParametersList(std::string_view boardName)
{
    std::list<std::string> statusList;
    for (const auto& parameter : m_templateBoards.at(boardName.data())->getParameters()) {
        if (parameter.second.refreshType == Board::ParameterInfo::RefreshType::SYNC) {
            statusList.emplace_back(parameter.first);
        }
    }
    return std::move(statusList);
}
