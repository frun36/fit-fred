#include<algorithm>
#include<string>
#include"FitData.h"
#include "Alfred/print.h"
#include"utils.h"
#include<sstream>
#include<iomanip>

FitData::FitData():m_ready(false)
{
    Print::PrintInfo("Fetching configuration from data base - start");
    if(DatabaseInterface::isConnected() == false)
    {
        Print::PrintError("DB connection failed");
        return;
    }

    Print::PrintInfo("Fetching TCM register map");
    auto parametersTCM = DatabaseInterface::executeQuery(
                        "SELECT * FROM BOARD_PARAMETERS WHERE BOARD_TYPE = 'TCM'"   
    );
    Print::PrintInfo("Fetched " + std::to_string(parametersTCM.size()) + " rows");
    m_templateBoards.emplace("TCM", parseTemplateBoard(parametersTCM));
    if(parametersTCM.size() == 0)
    {
        Print::PrintError("TCM register data have not been found!");
        return;
    }

    Print::PrintInfo("Fetching PM register map");
    auto parametersPM = DatabaseInterface::executeQuery(
                       "SELECT * FROM BOARD_PARAMETERS WHERE BOARD_TYPE = 'PM'" );
    Print::PrintInfo("Fetched " + std::to_string(parametersPM.size()) + " rows");
    if(parametersPM.size() == 0)
    {
        Print::PrintError("PM register data have not been found!");
        return;
    }
    m_templateBoards.emplace("PM", parseTemplateBoard(parametersPM));


    //Print::PrintInfo("Fetching Histogram register map");
    //auto parameterstHistogramPM = DatabaseInterface::executeQuery(
    //                    "SELECT * FROM PARAMETERS WHERE BOARD_TYPE = 'PM_HIST'"   
    //);
    //m_templateBoards.emplace("PM_HIST", parseTemplateBoard(parameterstHistogramPM)); 

    //if(parameterstHistogramPM.size() == 0)
    //{
    //    Print::PrintError("Histogram PM register data have not been found!");
    //    return;
    //}

    Print::PrintInfo("Fetching information about connected devices");
    auto connectedDevices = DatabaseInterface::executeQuery(
                        "SELECT * FROM CONNECTED_DEVICES"   
    );
    Print::PrintInfo("Fetched " + std::to_string(connectedDevices.size()) + " rows");
    if(connectedDevices.size() == 0)
    {
        Print::PrintError("Lacking data about connected devices");
        return;
    }

    Print::PrintInfo("Fetching information about unit defintion and others settings");
    auto settings = DatabaseInterface::executeQuery(
                "SELECT * FROM FEE_SETTINGS"
    );
    Print::PrintInfo("Fetched " + std::to_string(settings.size()) + " rows");
    parseSettings(settings);

    std::shared_ptr<Board> TCM{nullptr};

    for(auto& deviceRow : connectedDevices)
    {
        ConnectedDevicesTable::Device device(deviceRow);
	    Print::PrintInfo("Registering " + device.name);
        switch (device.type)
        {
        case ConnectedDevicesTable::Device::BoardType::PM:
        {
            if(TCM.get() == nullptr)
            {
                Print::PrintVerbose("PM row occured before TCM row!");
                return;
            }

            switch (device.side)
            {
            case ConnectedDevicesTable::Device::Side::A:
                m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index*AddressSpaceSizePM + BaseAddressPMA, m_templateBoards["PM"], TCM));
                break;
            
            case ConnectedDevicesTable::Device::Side::C:
                 m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index*AddressSpaceSizePM + BaseAddressPMC, m_templateBoards["PM"], TCM));
                 break;
            }

        }
        break;

        case ConnectedDevicesTable::Device::BoardType::TCM:
        {
            TCM = m_boards.emplace(device.name, constructBoardFromTemplate(device.name, 0x0, m_templateBoards["TCM"])).first->second;
        }
        break;
        }
    }

    m_ready = true;
}

Equation FitData::parseEquation(std::string equation)
{
    Equation parsed;
    size_t left = 0;
    size_t right = 0;
    while((left = equation.find('{', left)) != std::string::npos){
        if((right = equation.find('}', left)) == std::string::npos)
        {
            throw std::runtime_error("Invalid equation, missing }");
        }
        parsed.variables.emplace_back(equation.substr(left+1, right-left-1));
        left=right;
    }
    equation.erase(std::remove(equation.begin(), equation.end(),'{'), equation.end());
    equation.erase(std::remove(equation.begin(), equation.end(),'}'), equation.end());
    parsed.equation = std::move(equation);
    return parsed;
}

std::shared_ptr<Board> FitData::parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable)
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TemplateBoard", 0x0);
    for(auto& row : boardTable)
    {
	Print::PrintVerbose("Parsing parameter: " + row[ParametersTable::Parameter::Name]->getString());
        board->emplace(ParametersTable::Parameter::buildParameter(row));
    }
    Print::PrintVerbose("Board parsed successfully");
    return board;
}

std::shared_ptr<Board> FitData::constructBoardFromTemplate(std::string name, uint32_t address, std::shared_ptr<Board> templateBoard, std::shared_ptr<Board> main)
{
    std::shared_ptr<Board> board = std::make_shared<Board>(name, address, main, m_settings);
    for(const auto& parameter: templateBoard->getParameters())
    {
        board->emplace(parameter.second);
    }

    return board;
}

void FitData::parseSettings(std::vector<std::vector<MultiBase*>>& settingsTable)
{
    m_settings = std::make_shared<EnvironmentFEE>();
    for(auto& row: settingsTable)
    {
        std::string name = row[SettingsTable::Variable::Name]->getString();
        Equation equation =  parseEquation(row[SettingsTable::Variable::Equation]->getString());
        Print::PrintVerbose("Parsing " + name);
        Print::PrintVerbose("Equation: " + equation.equation);
        m_settings->emplace(
            EnvironmentFEE::Variable(name, equation)
            );
    }
    for(auto& row: settingsTable)
    {
        std::string name = row[SettingsTable::Variable::Name]->getString();
        Print::PrintVerbose("Updating " + name);
        m_settings->updateSetting(name);
        Print::PrintVerbose("Updated " + name + " to " + std::to_string(m_settings->getSetting(name)));
    }
}


///
///     ParameteresTable
///

bool ParametersTable::parseBoolean(MultiBase* field){
    return ( field->getString() == Parameter::ExprTRUE );
}

uint32_t ParametersTable::parseHex(MultiBase* field){
    std::stringstream ss;
    ss << field->getString();
    std::string hex(field->getString());
    uint32_t word=0;
    ss >> std::hex >> word;
    return word;
}

Board::ParameterInfo ParametersTable::Parameter::buildParameter(std::vector<MultiBase*>& dbRow)
{
    Equation electronicToPhysic = (dbRow[Parameter::EqElectronicToPhysic] == NULL) ?  
                        Equation::Empty() 
                        : FitData::parseEquation(dbRow[Parameter::EqElectronicToPhysic]->getString());

    Equation physicToElectronic = (dbRow[Parameter::EqPhysicToElectronic] == NULL) ?  
                        Equation::Empty() 
                        : FitData::parseEquation(dbRow[Parameter::EqPhysicToElectronic]->getString());

    Board::ParameterInfo::RefreshType refreshType =  Board::ParameterInfo::RefreshType::NOT;

    if(dbRow[Parameter::RefreshType] != NULL){

    if(dbRow[Parameter::RefreshType]->getString() == RefreshCNT){
        refreshType = Board::ParameterInfo::RefreshType::CNT;
    }
    else if (dbRow[Parameter::RefreshType]->getString() == RefreshSYNC){
        refreshType = Board::ParameterInfo::RefreshType::SYNC;
    }
    
    }

    Board::ParameterInfo::ValueEncoding encoding = parseBoolean(dbRow[IsSigned]) ? 
                        Board::ParameterInfo::ValueEncoding::Signed :
                        Board::ParameterInfo::ValueEncoding::Unsigned;

    uint32_t bitLength = dbRow[Parameter::EndBit]->getInt() - dbRow[Parameter::StartBit]->getInt() + 1;

    double max = 0;
    double min = 0;
    if(dbRow[Parameter::MinValue] == NULL){
        min = static_cast<double>(
            (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? 
                    0: twosComplementDecode<int32_t>((1u<< (bitLength - 1)), bitLength)
                    );
    }
    else{
        min = dbRow[Parameter::MinValue]->getDouble();
    }

    if(dbRow[Parameter::MaxValue] == NULL){
        max = static_cast<double>(
            (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? 
                (1u<<bitLength - 1): twosComplementDecode<int32_t>(1u<< (bitLength - 1) - 1, bitLength)
                );
    }
    else{
        max = dbRow[Parameter::MaxValue]->getDouble();
    }

    return {
            dbRow[Parameter::Name]->getString(),
            parseHex(dbRow[Parameter::BaseAddress]),
            static_cast<uint32_t>(dbRow[Parameter::StartBit]->getInt()),
            static_cast<uint8_t>(bitLength),
            static_cast<uint8_t>(dbRow[Parameter::RegBlockSize]->getInt()),
            encoding,
            min,
            max,
            electronicToPhysic,
            physicToElectronic,
            parseBoolean(dbRow[Parameter::IsFifo]),
            parseBoolean(dbRow[Parameter::IsReadOnly]),
            refreshType
            };
    
}


///
///     ConnectedDevicesTable
/// 

ConnectedDevicesTable::Device::Device(std::vector<MultiBase*>& dbRow)
{
    name = dbRow[Name]->getString();
    std::string type = dbRow[Type]->getString();

    if(type == TypePM) this->type = BoardType::PM;
    else if(type == TypeTCM) this->type = BoardType::TCM;
    else throw std::runtime_error("Invalid board type in Connected Devices table");

    if(this->type == BoardType::PM)
    {
        if(name.find("C") != std::string::npos) this->side = Side::C;
        else this->side = Side::A;

        this->index = std::stoi(name.substr(3,1));
    }
}
