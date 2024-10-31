#include "BasicRequestHandler.h"
#include "Board.h"
#include "utils.h"
#include "Alfred/print.h"

void BasicRequestHandler::resetExecutionData()
{
    m_registerTasks.clear();
    m_operations.clear();
}

SwtSequence BasicRequestHandler::processMessageFromWinCC(std::string mess, bool readAfterWrite)
{
    resetExecutionData();
    SwtSequence sequence;

    try {
        WinCCRequest request(mess);

        for (const auto& cmd : request.getCommands()) {

            SwtSequence::SwtOperation operation = createSwtOperation(cmd);
            Board::ParameterInfo& info = m_board->at(cmd.name);

            m_registerTasks[info.baseAddress].emplace_back(info.name, cmd.value);

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

void BasicRequestHandler::mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge)
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

SwtSequence::SwtOperation BasicRequestHandler::createSwtOperation(const WinCCRequest::Command& command) const
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

    int64_t electronicValue = m_board->calculateElectronic(parameter.name, command.value.value());
    if(electronicValue > parameter.maxValue || electronicValue < parameter.minValue){
        throw std::runtime_error(parameter.name + ": attempted to write a value outside the valid range - value: " + std::to_string(electronicValue));
    }

    uint32_t rawValue = m_board->convertElectronicToRaw(parameter.name, electronicValue);

    if (parameter.bitLength == 32) {
        return SwtSequence::SwtOperation(SwtSequence::Operation::Write, parameter.baseAddress, { rawValue });
    }

    return SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, parameter.baseAddress,
                                     { SwtSequence::createANDMask(parameter.startBit, parameter.bitLength), rawValue });
}

BasicRequestHandler::ParsedResponse BasicRequestHandler::processMessageFromALF(std::string alfresponse)
{
    WinCCResponse response;
    std::list<ErrorReport> report;

    try {
        AlfResponseParser alfMsg(alfresponse);
        if (!alfMsg.isSuccess()) {
            report.emplace_back("SEQUENCE", "ALF COMMUNICATION FAILED");
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

void BasicRequestHandler::unpackReadResponse(const AlfResponseParser::Line& read, WinCCResponse& response, std::list<ErrorReport>& report)
{
    for (auto& parameterToHandle : m_registerTasks.at(read.frame.address)) {
        try {
            double value = m_board->calculatePhysical(parameterToHandle.name, read.frame.data);
            if (parameterToHandle.toCompare.has_value() && value != parameterToHandle.toCompare.value()) {
                report.emplace_back(
                    parameterToHandle.name,
                    "WRITE FAILED: Received " + std::to_string(value) +
                        ", Expected " + std::to_string(parameterToHandle.toCompare.value()));
                m_board->at(parameterToHandle.name).storeValue(value);
                continue;
            }
            response.addParameter(parameterToHandle.name, { value });
            m_board->at(parameterToHandle.name).storeValue(value);

        } catch (const std::exception& e) {
            report.emplace_back(parameterToHandle.name, e.what());
        }
    }
}