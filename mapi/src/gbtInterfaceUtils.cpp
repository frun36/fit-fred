#include "gbtInterfaceUtils.h"
#include "Alfred/print.h"
#include <cstring>
#include<fstream>
#include<ctime>
#include"utils.h"

namespace
{
uint32_t build32(const uint8_t* buffer, size_t& idx){
    uint32_t word = static_cast<uint32_t>(static_cast<uint32_t>(idx) << 23u) +
            static_cast<uint32_t>(static_cast<uint32_t>(idx+1) << 15u) +
            static_cast<uint32_t>(static_cast<uint32_t>(idx+2) << 7u) +
            static_cast<uint32_t>(idx+3);
    idx += 4;
    return word;
}

uint16_t build16(const uint8_t* buffer, size_t& idx){
    uint16_t word = static_cast<uint32_t>(static_cast<uint32_t>(idx) << 7u) + static_cast<uint32_t>(idx+1);
    idx += 2;
    return word;
}

std::string createTimestamp() {
    std::time_t currentTime = std::time(nullptr);
    std::tm* localTime = std::localtime(&currentTime);

    // Create a string stream to format the timestamp
    std::ostringstream oss;
    oss << "[" << std::setfill('0')
        << std::setw(2) << localTime->tm_mday << '/'
        << std::setw(2) << (localTime->tm_mon + 1) << '/'
        << (localTime->tm_year + 1900) << ' '
        << std::setw(2) << localTime->tm_hour << ':'
        << std::setw(2) << localTime->tm_min << ':'
        << std::setw(2) << localTime->tm_sec << "]\t";

    return oss.str();
}
}

namespace gbt
{


[[nodiscard]] std::shared_ptr<GbtErrorType> parseFifoData(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    switch (fifoData[0]) {
        case BCSyncLost::getErrorCode():
            return std::make_shared<BCSyncLost>(fifoData);

        case FifoOverload::getErrorCode():
            return std::make_shared<FifoOverload>(fifoData);

        case PmEarlyHeader::getErrorCode():
            return std::make_shared<PmEarlyHeader>(fifoData);

        default:
            Print::PrintError("Unsupported GBT ERROR!");
            return std::make_shared<Unknown>(fifoData);
    }
}


Unknown::Unknown(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t pos = 0;
    for(size_t idx = 0; idx < constants::FifoSize; ){
        data[pos++] = build32(fifoData.data(), idx);
    }
}

[[nodiscard]] WinCCResponse Unknown::createWinCCResponse()
{
    WinCCResponse respone;
    respone.addParameter("GBT_ERR_UNKNOWN", { 1 });
    return respone;
}

void Unknown::saveErrorReport()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);

    file << "\n";
    std::string timeStamp = createTimestamp();

    file << timeStamp << "Unknown error report: ## IPbusWord" << std::endl;
    for(int idx = 0; idx < data.size(); idx++){
        file << timeStamp <<  std::setw(2) << std::setfill('0') << idx << " " << std::hex << data[idx] << std::endl;
    }
}

BCSyncLost::BCSyncLost(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t index = 0;

    data.errorCode = build32(fifoData.data(), index);

    for (int i = 0; i < 10; ++i) {
        for(int p = 0; p < GbtWord::WordSize; p++){
            data.words[i].data.buffer[p] = build32(fifoData.data(),index);
        }

        data.words[i].counter = build16(fifoData.data(), index);
        data.words[i].isData  = fifoData[index++];
    }

    data.BCCRU       = build16(fifoData.data(),index);
    data.BCBoard     = build16(fifoData.data(),index);
    data.orbitBoard  = build32(fifoData.data(),index);
    data.orbitCRU    = build32(fifoData.data(),index);
    data.reservedSpace[0] = build32(fifoData.data(),index);
    data.reservedSpace[1] = build32(fifoData.data(),index);
}

WinCCResponse BCSyncLost::createWinCCResponse()
{
    WinCCResponse response;
    response.addParameter(gbt::parameters::BCSyncLost.data(), { 1 });
    return std::move(response);
}

void BCSyncLost::saveErrorReport()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);

    file << "\n";
    std::string timeStamp = createTimestamp();

    file << timeStamp << "Board BCid: " << std::hex << data.orbitBoard << " " << std::hex << data.BCBoard << std::endl;
    file << timeStamp << "\tCRU BCid: " << std::hex << data.orbitCRU << " " << std::hex << data.BCCRU << std::endl;
    file << timeStamp << "#### isData GBTword" << std::endl;
    for(int i = 0; i < 10; i++){
        file << timeStamp << std::setw(4) << data.words[i].counter << " " << std::setw(6) << (data.words[i].isData ? "true" : "false");
        for(int j = 0; j < GbtWord::WordSize; j++){
            file << std::hex << data.words[i].data.buffer[j] << ' ';
        }
        file << std::endl;
    }
}

PmEarlyHeader::PmEarlyHeader(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t index = 0;
    data.errorCode = build32(fifoData.data(), index);
    for(int i = 0; i < 14; i++){
        for(int p = 0; p < GbtWord::WordSize; p++){
            data.words[i].buffer[p] = build32(fifoData.data(), index);
        }
    }
}

WinCCResponse PmEarlyHeader::createWinCCResponse()
{
    WinCCResponse response;
    response.addParameter(gbt::parameters::PmEarlyHeader.data(), { 1 });
    return std::move(response);
}

void PmEarlyHeader::saveErrorReport()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);
    
    file << "\n";
    std::string timeStamp = createTimestamp();

    file << timeStamp <<"Input packet corrupted: header too early ## GBTword" << std::endl;
    for(int i = 0; i < 14; i++){
        file << timeStamp << std::setw(2) << std::setfill('0') << i << " ";
        for(int j = 0; j < GbtWord::WordSize; j++){
            file << std::hex << data.words[i].buffer[j] << ' ';
        }
        file << std::endl;
    }
}

FifoOverload::FifoOverload(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t index = 0;

    data.errorCode = build32(fifoData.data(), index);
    for(int i = 0; i<13; i++){
        for(int p = 0; p < GbtWord::WordSize; p++){
            data.words[i].buffer[p] = build32(fifoData.data(), index);
        }
    }

    data.reservedSpace0 = build16(fifoData.data(), index);
    data.rdRate = build16(fifoData.data(), index);
    data.wrRate = build16(fifoData.data(), index);
    data._reservedSpace1 = build16(fifoData.data(), index);
}

WinCCResponse FifoOverload::createWinCCResponse()
{
    WinCCResponse response;
    response.addParameter(gbt::parameters::FifoOverload.data(), { 1 });
    return std::move(response);
}

void FifoOverload::saveErrorReport()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);
    
    file << "\n";
    std::string timeStamp = createTimestamp();

    file << timeStamp << "Raw data FIFO overload" << std::endl;
    file << timeStamp << data.rdRate << " read and " << data.wrRate << " operations in last 1000 cycles" << std::endl;
    file << timeStamp << "## GBT word" << std::endl;
    for(int idx = 0; idx < 13; idx++){
        file << timeStamp << std::setw(2) << std::setfill('0') << idx << " ";
        for(int j = 0; j < GbtWord::WordSize; j++){
            file << std::hex << data.words[idx].buffer[j] << ' ';
        }
    }
}

} // namespace gbt

namespace gbt
{
WinCCResponse GbtRateMonitor::updateRates(uint32_t wordsCount, uint32_t eventsCount)
{
    WinCCResponse response;
    response.addParameter(parameters::WordsRate.data(), { m_wordsRate.updateRate(wordsCount, m_timeInterval) });
    response.addParameter(parameters::EventsRate.data(), { m_eventsRate.updateRate(eventsCount, m_timeInterval) });
    return response;
}

double GbtRate::updateRate(uint32_t newCount, std::chrono::milliseconds timeInterval)
{
    double rate = (newCount >= oldCount) ? static_cast<double>(newCount - oldCount) / static_cast<double>(timeInterval.count()) : newCount;
    oldCount = newCount;
    return rate;
}

} // namespace gbt