#include <string>
#include "FitData.h"
#include "Alfred/print.h"
#include "utils.h"
#include "Database/sql.h"
#include <sstream>
#include <iomanip>

///

FitData::DeviceInfo::DeviceInfo(std::vector<MultiBase*>& dbRow)
{
    name = dbRow[db_tables::ConnectedDevices::BoardName.idx]->getString();
    std::string type = dbRow[db_tables::ConnectedDevices::BoardType.idx]->getString();

    if (type == db_tables::ConnectedDevices::TypePM)
        this->type = BoardType::PM;
    else if (type == db_tables::ConnectedDevices::TypeTCM)
        this->type = BoardType::TCM;
    else
        throw std::runtime_error("Invalid board type in Connected Devices table");

    if (this->type == BoardType::PM) {
        if (name.find("C") != std::string::npos)
            this->side = Side::C;
        else
            this->side = Side::A;

        this->index = std::stoi(name.substr(3, 1));
    }
}

FitData::FitData() : m_ready(false)
{
    Print::PrintInfo("Fetching configuration from data base - start");
    if (DatabaseInterface::isConnected() == false) {
        Print::PrintError("DB connection failed");
        return;
    }

    if (!fetchBoardParamters(db_tables::BoardTypes::TypeTCM)) {
        return;
    }
    if (!fetchBoardParamters(db_tables::BoardTypes::TypePM)) {
        return;
    }
    if (!fetchEnvironment()) {
        return;
    }
    if (!fetchConnectedDevices()) {
        return;
    }

    m_ready = true;
}

bool FitData::fetchBoardParamters(std::string boardType)
{
    sql::SelectModel query;
    query.select("*").from(db_tables::Parameters::TableName).where(sql::column(db_tables::Parameters::BoardType.name) == boardType);

    Print::PrintInfo("Fetching" + boardType + " register map");
    auto parameters = DatabaseInterface::executeQuery(query.str());
    Print::PrintInfo("Fetched " + std::to_string(parameters.size()) + " rows");

    if (parameters.size() == 0) {
        Print::PrintError(boardType + " register data have not been found!");
        return false;
    }

    m_templateBoards.emplace(boardType, parseTemplateBoard(parameters));
    m_statusParameters.emplace(boardType, constructStatusParametersList(boardType));
    return true;
}

bool FitData::fetchEnvironment()
{
    Print::PrintInfo("Fetching information about unit defintion and others environement variables");
    sql::SelectModel query;
    query.select("*").from(db_tables::Environment::TableName);

    auto environment = DatabaseInterface::executeQuery(query.str());
    Print::PrintInfo("Fetched " + std::to_string(environment.size()) + " rows");
    parseEnvVariables(environment);
    if (!checkEnvironment()) {
        return false;
    }

    return true;
}

bool FitData::fetchConnectedDevices()
{
    sql::SelectModel query;
    query.select("*").from(db_tables::ConnectedDevices::TableName);

    Print::PrintInfo("Fetching information about connected devices");
    auto connectedDevices = DatabaseInterface::executeQuery(query.str());

    Print::PrintInfo("Fetched " + std::to_string(connectedDevices.size()) + " rows");
    if (connectedDevices.size() == 0) {
        Print::PrintError("Lacking data about connected devices");
        return false;
    }

    std::shared_ptr<Board> TCM{ nullptr };

    for (auto& deviceRow : connectedDevices) {
        DeviceInfo device(deviceRow);
        Print::PrintInfo("Registering " + device.name);

        switch (device.type) {
            case DeviceInfo::BoardType::PM: {
                if (TCM.get() == nullptr) {
                    Print::PrintVerbose("PM row occured before TCM row!");
                    return false;
                }

                switch (device.side) {
                    case DeviceInfo::Side::A:
                        m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index * AddressSpaceSizePM + BaseAddressPMA, m_templateBoards["PM"], TCM));
                        break;

                    case DeviceInfo::Side::C:
                        m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index * AddressSpaceSizePM + BaseAddressPMC, m_templateBoards["PM"], TCM));
                        break;
                }

            } break;

            case DeviceInfo::BoardType::TCM: {
                TCM = m_boards.emplace(device.name, constructBoardFromTemplate(device.name, 0x0, m_templateBoards["TCM"])).first->second;
            } break;
        }
    }

    return true;
}

std::shared_ptr<Board> FitData::parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable)
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TemplateBoard", 0x0);
    for (auto& row : boardTable) {
        Print::PrintVerbose("Parsing parameter: " + row[db_tables::Parameters::Name.idx]->getString());
        board->emplace(parseParameter(row));
    }
    Print::PrintVerbose("Board parsed successfully");
    return board;
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

Board::ParameterInfo FitData::parseParameter(std::vector<MultiBase*>& dbRow)
{
    Equation electronicToPhysic = (dbRow[db_tables::Parameters::EqElectronicToPhysic.idx] == NULL) ? Equation::Empty()
                                                                                                   : db_tables::equation::parseEquation(dbRow[db_tables::Parameters::EqElectronicToPhysic.idx]->getString());

    Equation physicToElectronic = (dbRow[db_tables::Parameters::EqPhysicToElectronic.idx] == NULL) ? Equation::Empty()
                                                                                                   : db_tables::equation::parseEquation(dbRow[db_tables::Parameters::EqPhysicToElectronic.idx]->getString());

    Board::ParameterInfo::RefreshType refreshType = Board::ParameterInfo::RefreshType::NOT;

    if (dbRow[db_tables::Parameters::RefreshType.idx] != NULL) {

        if (dbRow[db_tables::Parameters::RefreshType.idx]->getString() == db_tables::Parameters::RefreshCNT) {
            refreshType = Board::ParameterInfo::RefreshType::CNT;
        } else if (dbRow[db_tables::Parameters::RefreshType.idx]->getString() == db_tables::Parameters::RefreshSYNC) {
            refreshType = Board::ParameterInfo::RefreshType::SYNC;
        }
    }

    Board::ParameterInfo::ValueEncoding encoding = db_tables::boolean::parse(dbRow[db_tables::Parameters::IsSigned.idx]) ? Board::ParameterInfo::ValueEncoding::Signed : Board::ParameterInfo::ValueEncoding::Unsigned;
    uint32_t startBit = dbRow[db_tables::Parameters::StartBit.idx]->getInt();
    uint32_t bitLength = dbRow[db_tables::Parameters::EndBit.idx]->getInt() - startBit + 1;

    int64_t max = 0;
    int64_t min = 0;
    if (dbRow[db_tables::Parameters::MinValue.idx] == NULL) {
        min = (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? 0 : (1u << (bitLength - 1));
    } else {
        min = static_cast<int64_t>(dbRow[db_tables::Parameters::MinValue.idx]->getDouble());
    }

    if (dbRow[db_tables::Parameters::MaxValue.idx] == NULL) {
        max = (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? ((1u << bitLength) - 1): (1u << (bitLength - 1));
    } else {
        max =  static_cast<int64_t>(dbRow[db_tables::Parameters::MaxValue.idx]->getDouble());
    }

    return {
        dbRow[db_tables::Parameters::Name.idx]->getString(),
        db_tables::hex::parse(dbRow[db_tables::Parameters::BaseAddress.idx]),
        startBit,
        bitLength,
        static_cast<uint8_t>(dbRow[db_tables::Parameters::RegBlockSize.idx]->getInt()),
        encoding,
        min,
        max,
        electronicToPhysic,
        physicToElectronic,
        db_tables::boolean::parse(dbRow[db_tables::Parameters::IsFifo.idx]),
        db_tables::boolean::parse(dbRow[db_tables::Parameters::IsReadOnly.idx]),
        refreshType
    };
}

std::shared_ptr<Board> FitData::constructBoardFromTemplate(std::string name, uint32_t address, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main)
{
    std::shared_ptr<Board> board = std::make_shared<Board>(name, address, main, m_environmentalVariables);
    for (const auto& parameter : templateBoard->getParameters()) {
        board->emplace(parameter.second);
    }

    return board;
}

void FitData::parseEnvVariables(std::vector<std::vector<MultiBase*>>& settingsTable)
{
    m_environmentalVariables = std::make_shared<EnvironmentVariables>();
    for (auto& row : settingsTable) {
        std::string name = row[db_tables::Environment::Name.idx]->getString();
        Equation equation = db_tables::equation::parseEquation(row[db_tables::Environment::Equation.idx]->getString());
        Print::PrintVerbose("Parsing " + name);
        Print::PrintVerbose("Equation: " + equation.equation);
        m_environmentalVariables->emplace(
            EnvironmentVariables::Variable(name, equation));
        if (equation.variables.empty()) {
            m_environmentalVariables->updateVariable(name);
        }
    }
    for (auto& row : settingsTable) {
        std::string name = row[db_tables::Environment::Name.idx]->getString();
        Print::PrintVerbose("Updating " + name);
        m_environmentalVariables->updateVariable(name);
        Print::PrintVerbose("Updated " + name + " to " + std::to_string(m_environmentalVariables->getVariable(name)));
    }
}

bool FitData::checkEnvironment()
{
    if (!m_environmentalVariables->doesExist(environment::parameters::ExtenalClock.data())) {
        Print::PrintError(std::string(environment::parameters::ExtenalClock.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::InternalClock.data())) {
        Print::PrintError(std::string(environment::parameters::InternalClock.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::SystemClock.data())) {
        Print::PrintError(std::string(environment::parameters::SystemClock.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::TDC.data())) {
        Print::PrintError(std::string(environment::parameters::TDC.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::PmA0BoardId.data())) {
        Print::PrintError(std::string(environment::parameters::PmA0BoardId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::PmC0BoardId.data())) {
        Print::PrintError(std::string(environment::parameters::PmC0BoardId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::TcmBoardId.data())) {
        Print::PrintError(std::string(environment::parameters::TcmBoardId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::SystemId.data())) {
        Print::PrintError(std::string(environment::parameters::SystemId.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::BcIdOffsetDefault.data())) {
        Print::PrintError(std::string(environment::parameters::BcIdOffsetDefault.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::Trigger1Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger1Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::Trigger2Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger2Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }
    if (!m_environmentalVariables->doesExist(environment::parameters::Trigger3Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger3Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }
    if (!m_environmentalVariables->doesExist(environment::parameters::Trigger4Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger4Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    if (!m_environmentalVariables->doesExist(environment::parameters::Trigger5Signature.data())) {
        Print::PrintError(std::string(environment::parameters::Trigger5Signature.data()) + " env variable does not exist! Aborting!");
        return false;
    }

    return true;
}