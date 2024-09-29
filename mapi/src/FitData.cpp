#include<algorithm>
#include<string>
#include"FitData.h"
#include "Alfred/print.h"

FitData::FitData():m_ready(false)
{
    Print::PrintInfo("Fetching configuration from data base - start");
    if(DatabaseInterface::isConnected() == false)
    {
        Print::PrintError("DB connection failed");
        return;
    }

    Print::PrintInfo("Fetching PM register map");
    auto parametersPM = DatabaseInterface::executeQuery(
                        "SELECT * FROM PARAMETERS WHERE BOARD_TYPE EQUALS \"PM\"" );
    Print::PrintInfo("Fetched " + std::to_string(parametersPM.size()) + " rows");
    if(parametersPM.size() == 0)
    {
        Print::PrintError("PM register data have not been found!");
        return;
    }
    m_templateBoards.emplace("PM", parseTemplateBoard(parametersPM));

    Print::PrintInfo("Fetching TCM register map");
    auto parametersTCM = DatabaseInterface::executeQuery(
                        "SELECT * FROM PARAMETERS WHERE BOARD_TYPE EQUALS \"TCM\""   
    );
    m_templateBoards.emplace("TCM", parseTemplateBoard(parametersTCM));

    if(parametersTCM.size() == 0)
    {
        Print::PrintError("TCM register data have not been found!");
        return;
    }

    Print::PrintInfo("Fetching Histogram register map");
    auto parameterstHistogramPM = DatabaseInterface::executeQuery(
                        "SELECT * FROM PARAMETERS WHERE BOARD_TYPE EQUALS \"PM_HIST\""   
    );
    m_templateBoards.emplace("PM_HIST", parseTemplateBoard(parameterstHistogramPM)); 

     if(parameterstHistogramPM.size() == 0)
    {
        Print::PrintError("Histogram PM register data have not been found!");
        return;
    }

    Print::PrintInfo("Fetching information about connected devices");
    auto connectedDevices = DatabaseInterface::executeQuery(
                        "SELECT * FROM CONNECTED_DEVICES"   
    );

    if(connectedDevices.size() == 0)
    {
        Print::PrintError("Lacking data about connected devices");
        return;
    }

    for(auto& deviceRow : connectedDevices)
    {
        ConnectedDevicesTable::Device device(deviceRow);
        switch (device.type)
        {
        case ConnectedDevicesTable::Device::BoardType::PM:
        {
            switch (device.side)
            {
            case ConnectedDevicesTable::Device::Side::A:
                m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index*AddressSpaceSizePM + BaseAddressPMA, m_templateBoards["PM"]));
                break;
            
            case ConnectedDevicesTable::Device::Side::C:
                 m_boards.emplace(device.name, constructBoardFromTemplate(device.name, device.index*AddressSpaceSizePM + BaseAddressPMC, m_templateBoards["PM"]));
                 break;
            }
        }
        break;

        case ConnectedDevicesTable::Device::BoardType::TCM:
        {
             m_boards.emplace(device.name, constructBoardFromTemplate(device.name, 0x0, m_templateBoards["TCM"]));
        }
        break;
        }
    }

    m_ready = true;
}

std::shared_ptr<Board> FitData::parseTemplateBoard(std::vector<std::vector<MultiBase*>>& boardTable)
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TemplateBoard", 0x0);
    for(auto& row : boardTable)
    {
        board->emplace(ParametersTable::Parameter::buildParameter(row));
    }
    return board;
}

std::shared_ptr<Board> FitData::constructBoardFromTemplate(std::string name, uint32_t address, std::shared_ptr<Board> templateBoard)
{
    std::shared_ptr<Board> board = std::make_shared<Board>(name, address);
    for(const auto& parameter: templateBoard->getParameters())
    {
        board->emplace(parameter.second);
    }

    return board;
}


///
///     ParameteresTable
///

bool ParametersTable::parseBoolean(MultiBase* field){
    return ( field->getString() == Parameter::ExprTRUE );
}

uint32_t ParametersTable::parseHex(MultiBase* field){
    std::string hex(std::move(field->getString()));
    uint32_t word = 0;
    uint32_t shift = 0;
    for(uint32_t pos = hex.length()-1; pos >= 0; pos--)
    {
        if (hex[pos] >= '0' && hex[pos] <= '9')
            word += (1u<<(4*pos)) * static_cast<uint32_t>(hex[pos] - '0');
        else if (hex[pos] >= 'A' && hex[pos] <= 'F')
            word += (1u<<(4*pos)) * static_cast<uint32_t>(hex[pos] - 'A' + 10);
        else if (hex[pos] >= 'a' && hex[pos] <= 'f')
            word += (1u<<(4*pos)) * static_cast<uint32_t>(hex[pos] - 'a' + 10);
        else
            throw std::invalid_argument("Invalid hexadecimal character");
    }
    return word;
}

Board::ParameterInfo ParametersTable::Parameter::buildParameter(std::vector<MultiBase*>& dbRow)
{
    Board::ParameterInfo::Equation electronicToPhysic = (dbRow[Parameter::EqElectronicToPhysic] == NULL) ?  
                        Board::ParameterInfo::Equation::Empty() 
                        : parseEquation(dbRow[Parameter::EqElectronicToPhysic]->getString());

    Board::ParameterInfo::Equation physicToElectronic = (dbRow[Parameter::EqPhysicToElectronic] == NULL) ?  
                        Board::ParameterInfo::Equation::Empty() 
                        : parseEquation(dbRow[Parameter::EqPhysicToElectronic]->getString());

    Board::ParameterInfo::RefreshType refreshType =  Board::ParameterInfo::RefreshType::NOT;

    if(dbRow[Parameter::RefreshType]->getString() == RefreshCNT){
        refreshType = Board::ParameterInfo::RefreshType::CNT;
    }
    else if (dbRow[Parameter::RefreshType]->getString() == RefreshSYNC){
        refreshType = Board::ParameterInfo::RefreshType::SYNC;
    }

    Board::ParameterInfo::ValueEncoding encoding = parseBoolean(dbRow[IsSigned]) ? 
                        Board::ParameterInfo::ValueEncoding::Signed :
                        Board::ParameterInfo::ValueEncoding::Unsigned;

    return {
            dbRow[Parameter::Name]->getString(),
            parseHex(dbRow[Parameter::BaseAddress]),
            static_cast<uint32_t>(dbRow[Parameter::StartBit]->getInt()),
            static_cast<uint8_t>(dbRow[Parameter::EndBit]->getInt()-dbRow[Parameter::StartBit]->getInt()+1),
            static_cast<uint8_t>(dbRow[Parameter::RegBlockSize]->getInt()),
            encoding,
            dbRow[Parameter::MinValue]->getDouble(),
            dbRow[Parameter::MaxValue]->getDouble(),
            electronicToPhysic,
            physicToElectronic,
            parseBoolean(dbRow[Parameter::IsFifo]),
            parseBoolean(dbRow[Parameter::IsReadOnly]),
            refreshType
            };
    
}

Board::ParameterInfo::Equation ParametersTable::Parameter::parseEquation(std::string equation)
{
    Board::ParameterInfo::Equation parsed;
    size_t left = 0;
    size_t right = 0;
    while((left = equation.find('{', left)) != std::string::npos){
        if((right = equation.find('}', left)) == std::string::npos)
        {
            throw std::runtime_error("Invalid equation, missing }");
        }
        parsed.variables.emplace_back(equation.substr(left+1, right-left-1));
    }
    equation.erase(std::remove(equation.begin(), equation.end(),'{'), equation.end());
    equation.erase(std::remove(equation.begin(), equation.end(),'}'), equation.end());
    parsed.equation = std::move(equation);
    return parsed;
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