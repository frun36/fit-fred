#include "gbtInterfaceUtils.h"
#include "Alfred/print.h"
#include <cstring>
#include<fstream>
#include<ctime>
#include"utils.h"

namespace
{

uint32_t build32(const uint8_t* buffer, size_t& idx){
    uint32_t word = static_cast<uint32_t>(static_cast<uint32_t>(buffer[idx+3]) << 24u) +
            static_cast<uint32_t>(static_cast<uint32_t>(buffer[idx+2]) << 16u) +
            static_cast<uint32_t>(static_cast<uint32_t>(buffer[idx+1]) << 8u) +
            static_cast<uint32_t>(buffer[idx+1]);
    idx += 4;
    return word;
}

uint16_t build16(const uint8_t* buffer, size_t& idx){
    uint16_t word = static_cast<uint16_t>(static_cast<uint16_t>(buffer[idx+1]) << 8u) + static_cast<uint16_t>(buffer[idx]);
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

std::ostream& spacing(std::ostream& os,size_t len)
{
    return (os << std::setw(len) << std::setfill(' ') << '\t');
}

std::ostream& operator<<(std::ostream& os, const GbtWord& word)
{
    os << std::hex << std::uppercase << word.buffer[4] << std::hex << word.buffer[3] << ' ';
    os << std::hex << std::uppercase << word.buffer[2] << ' ';
    os << std::hex << std::uppercase << word.buffer[1] << word.buffer[0];
    return os;
}

}

namespace gbt
{


[[nodiscard]] std::shared_ptr<GbtErrorType> parseFifoData(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    switch (fifoData[0]) {
        if(fifoData[0] == BCSyncLost::getErrorCode()){
            return std::make_shared<BCSyncLost>(fifoData);
        }

        if(fifoData[0] == FifoOverload::getErrorCode()){
            return std::make_shared<FifoOverload>(fifoData);
        }

        if(fifoData[0] == PmEarlyHeader::getErrorCode()){
            return std::make_shared<PmEarlyHeader>(fifoData);
        }
        return std::make_shared<Unknown>(fifoData);
    }
}


Unknown::Unknown(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    static_assert(sizeof(data) == constants::FifoSize * sizeof(uint32_t));
    std::memcpy((uint32_t*)&data, fifoData.data(), constants::FifoSize*sizeof(uint32_t));
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

    file << timeStamp << "Unknown error report: ## FIFO content" << std::endl;
    for(int idx = 0; idx < data.size(); idx++){
        spacing(file, timeStamp.length()) << std::setw(2) << std::setfill('0') << idx << " " << std::hex << data[idx] << std::endl;
    }
}

BCSyncLost::BCSyncLost(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    static_assert(sizeof(data) == constants::FifoSize * sizeof(uint32_t));
    std::memcpy((uint32_t*)&data, fifoData.data(), constants::FifoSize*sizeof(uint32_t));
    return;
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

    file << timeStamp << "Board BCid: " << std::hex << std::uppercase<< data.orbitBoard << " " << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << data.BCBoard << std::endl;
    spacing(file, timeStamp.length()) << "CRU BCid: " << std::hex << std::uppercase << data.orbitCRU << " " << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << data.BCCRU << std::endl;
    spacing(file, timeStamp.length()) << "#### isData GBTword" << std::endl;
    for(int i = 0; i < 10; i++){
        spacing(file, timeStamp.length()) << std::setw(4) << std::dec << data.words[i].counter << " " << std::setw(6) << (data.words[i].isData ? "true" : "false") << " ";
        file << data.words[i].data << std::endl;;
    }
}

PmEarlyHeader::PmEarlyHeader(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    static_assert(sizeof(data) == constants::FifoSize * sizeof(uint32_t));
    std::memcpy((uint32_t*)&data, fifoData.data(), constants::FifoSize*sizeof(uint32_t));
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
        spacing(file, timeStamp.length()) << std::setw(2) << std::setfill('0') << i << " ";
        file << data.words[i] << std::endl;
    }
}

FifoOverload::FifoOverload(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    static_assert(sizeof(data) == constants::FifoSize * sizeof(uint32_t));
    std::memcpy((uint32_t*)&data, fifoData.data(), constants::FifoSize*sizeof(uint32_t));
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
    spacing(file, timeStamp.length()) << data.rdRate << " read and " << data.wrRate << " operations in last 1000 cycles" << std::endl;
    spacing(file, timeStamp.length()) << "## GBT word" << std::endl;
    for(int idx = 0; idx < 13; idx++){
        spacing(file, timeStamp.length()) << std::setw(2) << std::setfill('0') << idx << " ";
        file << data.words[idx] << std::endl;
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