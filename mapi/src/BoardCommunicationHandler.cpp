#include "BoardCommunicationHandler.h"
#include "Board.h"
#include "utils.h"
#include "Alfred/print.h"

const BoardCommunicationHandler::ParsedResponse BoardCommunicationHandler::ParsedResponse::EmptyResponse({ WinCCResponse(), {} });
const BoardCommunicationHandler::FifoResponse BoardCommunicationHandler::FifoResponse::EmptyFifoResponse{ {}, nullopt };

void BoardCommunicationHandler::resetExecutionData()
{
    m_registerTasks.clear();
    m_operations.clear();
}

SwtSequence BoardCommunicationHandler::processMessageFromWinCC(std::string mess, bool readAfterWrite)
{
    resetExecutionData();
    SwtSequence sequence;

    try {
        WinCCRequest request(mess);

        for (const auto& cmd : request.getCommands()) {

            SwtSequence::SwtOperation operation = createSwtOperation(cmd);
            Board::ParameterInfo& info = m_board->at(cmd.name);

            m_registerTasks[info.baseAddress].emplace_back(info.name, (cmd.value.has_value()) ? ((operation.type == SwtSequence::Operation::Write) ? operation.data[0] : operation.data[1] >> info.startBit) : cmd.value);

            if (m_operations.find(info.baseAddress) != m_operations.end()) {
                mergeOperation(m_operations.at(info.baseAddress), operation);
            } else {
                m_operations.emplace(info.baseAddress, operation);
            }
        }

        for (auto& operation : m_operations) {
            sequence.addOperation(operation.second);
        }

        if (request.isWrite() && readAfterWrite) {
            for (auto& rmw : m_operations) {
                sequence.addOperation(SwtSequence::Operation::Read, rmw.first, {}, true);
            }
        }

    } catch (const std::exception& e) {
        resetExecutionData();
        throw std::runtime_error(e.what());
    }

    return sequence;
}

void BoardCommunicationHandler::mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge)
{
    if (operation.type != toMerge.type) {
        throw std::runtime_error("Cannot merge operation of diffrent types!");
    }
    switch (operation.type) {
        case SwtSequence::Operation::Read: {
            return;
        } break;
        case SwtSequence::Operation::RMWbits: {
            operation.data[0] &= toMerge.data[0];
            operation.data[1] |= toMerge.data[1];
        } break;
        default:
            throw std::runtime_error("Unsupported operation to merge");
            break;
    }
}

SwtSequence::SwtOperation BoardCommunicationHandler::createSwtOperation(const WinCCRequest::Command& command) const
{
    Board::ParameterInfo& parameter = m_board->at(command.name);
    if (parameter.regBlockSize != 1) {
        throw std::runtime_error(parameter.name + ": regblock operations unsupported");
    }
    if (command.operation == WinCCRequest::Operation::Read) {
        return SwtSequence::SwtOperation(SwtSequence::Operation::Read, parameter.baseAddress, {}, true);
    }

    if (parameter.isReadonly) {
        throw std::runtime_error(parameter.name + ": attempted WRITE on read-only parameter");
    }

    if (!command.value.has_value()) {
        throw std::runtime_error(parameter.name + ": no data for WRITE operation");
    }

    int64_t electronicValue = 0;
    if(command.operation == WinCCRequest::Operation::WriteElectronic){
        electronicValue = static_cast<int64_t>(command.value.value());
    }
    else{
        electronicValue = m_board->calculateElectronic(parameter.name, command.value.value());
    }
    
    if (electronicValue > parameter.maxValue || electronicValue < parameter.minValue) {
        throw std::runtime_error(parameter.name + ": attempted to write a value outside the valid range [" + std::to_string(parameter.minValue) + "; " + std::to_string(parameter.maxValue) + "] - value: " + std::to_string(electronicValue));
    }

    uint32_t rawValue = m_board->convertElectronicToRaw(parameter.name, electronicValue);

    if (parameter.bitLength == 32) {
        return SwtSequence::SwtOperation(SwtSequence::Operation::Write, parameter.baseAddress, { rawValue });
    }

    return SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, parameter.baseAddress,
                                     { SwtSequence::createANDMask(parameter.startBit, parameter.bitLength), rawValue });
}

BoardCommunicationHandler::ParsedResponse BoardCommunicationHandler::processMessageFromALF(std::string alfresponse)
{
    WinCCResponse response;
    std::list<ErrorReport> report;

    try {
        AlfResponseParser alfMsg(alfresponse);
        if (!alfMsg.isSuccess()) {
            report.emplace_back("SEQUENCE", "ALF COMMUNICATION FAILED - message: " + alfresponse);
            return { std::move(response), std::move(report) };
        }

        for (auto line : alfMsg) {
            switch (line.type) {
                case AlfResponseParser::Line::Type::ResponseToRead:
                    unpackReadResponse(line, response, report);
                    break;
                case AlfResponseParser::Line::Type::ResponseToWrite:
                    break;
                default:
                    throw std::runtime_error("Unsupported ALF response");
            }
        }

    } catch (const std::exception& e) {
        report.emplace_back("SEQUENCE", e.what());
        return { std::move(response), std::move(report) };
    }

    return { std::move(response), std::move(report) };
}

void BoardCommunicationHandler::unpackReadResponse(const AlfResponseParser::Line& read, WinCCResponse& response, std::list<ErrorReport>& report)
{
    for (auto& parameterToHandle : m_registerTasks.at(read.frame.address)) {
        try {
            int64_t electronic = m_board->parseElectronic(parameterToHandle.name, read.frame.data);
            double value = m_board->calculatePhysical(parameterToHandle.name, electronic);

            Board::ParameterInfo info = m_board->at(parameterToHandle.name);

            if (parameterToHandle.toCompare.has_value() && getBitField(read.frame.data, info.startBit, info.bitLength) != parameterToHandle.toCompare.value()) {
                report.emplace_back(
                    parameterToHandle.name,
                    "WRITE FAILED: Received " + std::to_string(value) +
                        ", Expected " + std::to_string(parameterToHandle.toCompare.value()));
            }
            response.addParameter(parameterToHandle.name, { value });
            m_board->at(parameterToHandle.name).storeValue(value, electronic);

        } catch (const std::exception& e) {
            report.emplace_back(parameterToHandle.name, e.what());
        }
    }
}

SwtSequence BoardCommunicationHandler::createReadFifoRequest(std::string fifoName, size_t wordsToRead)
{
    resetExecutionData();
    Board::ParameterInfo& fifoInfo = m_board->at(fifoName);
    SwtSequence request;
    for (size_t n = 0; n < wordsToRead; n++) {
        request.addOperation(SwtSequence::Operation::Read, fifoInfo.baseAddress, nullptr);
    }
    m_registerTasks[fifoInfo.baseAddress].emplace_back(fifoName, std::nullopt);
    return request;
}

BoardCommunicationHandler::FifoResponse BoardCommunicationHandler::parseFifo(std::string alfResponse)
{
    if (m_registerTasks.size() != 1) {
        throw std::runtime_error("Forbidden action - FIFO read has been interleaved with other operation!");
    }

    auto& fifoTasks = m_registerTasks.begin()->second;
    if (fifoTasks.size() != 1) {
        throw std::runtime_error("Forbidden action - FIFO read has been interleaved with other operation!");
    }

    auto& fifoTask = fifoTasks.front();
    if (fifoTask.toCompare.has_value()) {
        throw std::runtime_error("Tried to parsed response to write by parse FIFO method");
    }

    Board::ParameterInfo& fifoInfo = m_board->at(fifoTask.name);

    std::vector<std::vector<uint32_t>> fifo(1);
    AlfResponseParser parser(alfResponse);

    if (!parser.isSuccess()) {
        return { {}, ErrorReport{ "SEQUENCE", "ALF COMMUNICATION FAILED" } };
    }

    for (auto line : parser) {
        if (line.type == AlfResponseParser::Line::Type::ResponseToWrite) {
            continue;
        }
        if (fifo.back().size() == fifoInfo.regBlockSize) {
            fifo.emplace_back(std::vector<uint32_t>());
            fifo.back().reserve(fifoInfo.regBlockSize);
        }
        fifo.back().emplace_back(m_board->parseElectronic(fifoInfo.name, line.frame.data));
    }

    return { fifo, {} };
}

