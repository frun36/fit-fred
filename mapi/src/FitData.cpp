#include <string>
#include "FitData.h"
#include "Alfred/print.h"
#include "utils.h"
#include <sstream>
#include <iomanip>
#include"database/FitDataQueries.h"

///

FitData::DeviceInfo::DeviceInfo(std::vector<MultiBase*>& dbRow)
{
    name = dbRow[db_fit::tabels::ConnectedDevices::BoardName.idx]->getString();
    std::string type = dbRow[db_fit::tabels::ConnectedDevices::BoardType.idx]->getString();

    if (type == db_fit::tabels::ConnectedDevices::TypePM)
        this->type = BoardType::PM;
    else if (type == db_fit::tabels::ConnectedDevices::TypeTCM)
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
    isConnected = db_fit::tabels::ConnectedDevices::IsConnected.parse(dbRow[db_fit::tabels::ConnectedDevices::IsConnected.idx]);
}

FitData::FitData() : m_ready(false)
{
    Print::PrintInfo("Fetching configuration from data base - start");
    if (DatabaseInterface::isConnected() == false) {
        Print::PrintError("DB connection failed");
        return;
    }

    if (!fetchBoardParamters(db_fit::tabels::BoardTypes::TypeTCM)) {
        return;
    }
    if (!fetchBoardParamters(db_fit::tabels::BoardTypes::TypePM)) {
        return;
    }
    if (!fetchEnvironment()) {
        return;
    }
    if(!fetchPmHistogramStructure()){
        return;
    }
    if (!fetchConnectedDevices()) {
        return;
    }

    m_ready = true;
}

bool FitData::fetchBoardParamters(std::string boardType)
{
    Print::PrintInfo("Fetching " + boardType + " register map");
    auto parameters = DatabaseInterface::executeQuery(db_fit::queries::selectParameters(boardType));
    Print::PrintInfo("Fetched " + std::to_string(parameters.size()) + " rows");

    if (parameters.size() == 0) {
        Print::PrintError(boardType + " register data has not been found!");
        return false;
    }

    m_templateBoards.emplace(boardType, parseTemplateBoard(parameters));
    m_statusParameters.emplace(boardType, constructStatusParametersList(boardType));
    return true;
}

bool FitData::fetchEnvironment()
{
    Print::PrintInfo("Fetching information about unit definitions and other environment variables");
    
    auto environment = DatabaseInterface::executeQuery(db_fit::queries::selectEnvironment());
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
    auto connectedDevices = DatabaseInterface::executeQuery(db_fit::queries::selectConnectedDevices());

    Print::PrintInfo("Fetched " + std::to_string(connectedDevices.size()) + " rows");
    if (connectedDevices.size() == 0) {
        Print::PrintError("Lacking data about connected devices");
        return false;
    }

    std::shared_ptr<Board> TCM{ nullptr };

    for (auto& deviceRow : connectedDevices) {
        DeviceInfo device(deviceRow);
        if (device.type != DeviceInfo::BoardType::TCM) {
            continue;
        }
        Print::PrintInfo("Registering " + device.name);
        TCM = m_boards.emplace(device.name, constructBoardFromTemplate(device.name, 0x0, device.isConnected, m_templateBoards["TCM"])).first->second;
        m_environmentalVariables->emplace({device.name, Equation{"1",{}}});
    }

    if (TCM.get() == nullptr) {
            Print::PrintVerbose("Missing TCM!");
            return false;
    }

    for (auto& deviceRow : connectedDevices) {
        DeviceInfo device(deviceRow);
        if (device.type == DeviceInfo::BoardType::TCM) {
            continue;
        }

        if(!device.isConnected){
            Print::PrintWarning(device.name + " is not connected");
            m_environmentalVariables->emplace({device.name, Equation{"0",{}}});
        }
        else{
            m_environmentalVariables->emplace({device.name, Equation{"1",{}}});
        }
        
        Print::PrintInfo("Registering " + device.name);

        switch (device.side) {
            case DeviceInfo::Side::A:
                m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index * AddressSpaceSizePM + BaseAddressPMA, device.isConnected, m_templateBoards["PM"], TCM));
                break;

            case DeviceInfo::Side::C:
                m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index * AddressSpaceSizePM + BaseAddressPMC, device.isConnected, m_templateBoards["PM"], TCM));
                break;
        }
    }

    return true;
}

bool FitData::fetchPmHistogramStructure()
{
    auto histogramStructure = DatabaseInterface::executeQuery(db_fit::queries::selectPmHistograms());
    for(auto rawRow: histogramStructure)
    {
        db_fit::views::Histogram::Row row(rawRow);
        if(m_PmHistograms.find(row.histogramName) == m_PmHistograms.end()){
            m_PmHistograms.emplace(row.histogramName, PmHistogram()); 
        }
        auto& hist = m_PmHistograms[row.histogramName];
        if(row.startBin < 0){
            hist.negativeBins = PmHistogramBlock();
            hist.negativeBins->baseAddress = row.baseAddress;
            hist.negativeBins->regBlockSize = row.regBlockSize;
            hist.negativeBins->startBin = row.startBin;
            hist.negativeBins->direction = (row.direction == "P") ? PmHistogramBlock::Direction::Positive : PmHistogramBlock::Direction::Negative;
        }
        else{
            hist.positiveBins = PmHistogramBlock();
            hist.positiveBins->baseAddress = row.baseAddress;
            hist.positiveBins->regBlockSize = row.regBlockSize;
            hist.positiveBins->startBin = row.startBin;
            hist.positiveBins->direction = (row.direction == "P") ? PmHistogramBlock::Direction::Positive : PmHistogramBlock::Direction::Negative;
        }
    }

    for(auto [name, histogram]: m_PmHistograms)
    {
        if(histogram.negativeBins == std::nullopt){
            Print::PrintError("Information about " + name + " is incomplete; negative bins definition is missing");
            return false;
        }
        if(histogram.positiveBins == std::nullopt){
            Print::PrintError("Information about " + name + " is incomplete; positive bins definition is missing");
            return false;
        }
        Print::PrintInfo(name + " histogram data successfully fetched");
    }
    return true;
}

std::shared_ptr<Board> FitData::parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable)
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TemplateBoard", 0x0, false);
    for (auto& row : boardTable) {
        Print::PrintVerbose("Parsing parameter: " + row[db_fit::tabels::Parameters::Name.idx]->getString());
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
    db_fit::tabels::Parameters::Row row(dbRow);
    Board::ParameterInfo::RefreshType refreshType = row.refreshType.has_value() ? Board::ParameterInfo::RefreshType::SYNC : Board::ParameterInfo::RefreshType::NOT;

    Board::ParameterInfo::ValueEncoding encoding = row.isSigned ? Board::ParameterInfo::ValueEncoding::Signed : Board::ParameterInfo::ValueEncoding::Unsigned;
    uint32_t bitLength = row.endBit - row.startBit + 1;

    int64_t max = 0;
    int64_t min = 0;
    if (row.minValue.has_value() == false) {
        min = (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? 0 : -(1ull << (bitLength - 1));
    } else {
        min = row.minValue.value();
    }

    if (row.maxValue.has_value() == false) {
        max = (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? ((1ull << bitLength) - 1) : ((1ull << (bitLength - 1)) - 1);
    } else {
        max = row.maxValue.value();
    }

    return {
        dbRow[db_fit::tabels::Parameters::Name.idx]->getString(),
        row.baseAddress,
        row.startBit,
        bitLength,
        row.regBlockSize,
        encoding,
        min,
        max,
        row.eqElectronicToPhysic,
        row.eqPhysicToElectronic,
        row.isFifo,
        row.isReadOnly,
        refreshType
    };
}

std::shared_ptr<Board> FitData::constructBoardFromTemplate(std::string name, uint32_t address, bool isConnected, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main)
{
    std::shared_ptr<Board> board = std::make_shared<Board>(name, address, isConnected, main, m_environmentalVariables);
    for (const auto& parameter : templateBoard->getParameters()) {
        board->emplace(parameter.second);
    }

    return board;
}

void FitData::parseEnvVariables(std::vector<std::vector<MultiBase*>>& settingsTable)
{
    m_environmentalVariables = std::make_shared<EnvironmentVariables>();
    for (auto& row : settingsTable) {
        db_fit::tabels::Environment::Row parsedRow(row);
        
        Print::PrintVerbose("Parsing " + parsedRow.name);
        Print::PrintVerbose("Equation: " + parsedRow.equation.equation);
        m_environmentalVariables->emplace(
            EnvironmentVariables::Variable(parsedRow.name, parsedRow.equation));
        if (parsedRow.equation.variables.empty()) {
            m_environmentalVariables->updateVariable(parsedRow.name);
        }
    }
    for (auto& row : settingsTable) {
        std::string name = row[db_fit::tabels::Environment::Name.idx]->getString();
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
