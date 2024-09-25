#include"BasicRequestHandler.h"
#include"Board.h"
#include"AlfResponseParser.h"
#include"utils.h"

void BasicRequestHandler::resetExecutionData()
{
    m_registerTasks.clear();
    m_operations.clear();
}

SwtSequence& BasicRequestHandler::processMessageFromWinCC(std::string mess)
{
	resetExecutionData();
    try
    {
	WinCCRequest request(mess);

 	for (const auto& cmd : request.getCommands()) {

		SwtSequence::SwtOperation operation = createSwtOperation(cmd);
		Board::ParameterInfo& info = m_board->at(cmd.name);

		m_registerTasks[info.baseAddress].emplace_back(info.name, cmd.value);
		if(m_operations.find(info.baseAddress) != m_operations.end()){
			mergeOperation(m_operations.at(info.baseAddress), operation);
		}
		else{
			m_operations.emplace(info.baseAddress, operation);
		}	
	}

	if(request.isWrite()){
		for(auto& rmw: m_operations){
			m_sequence.addOperation(SwtSequence::Operation::Read, rmw.first, {}, true);
		}
	}
    }
    catch (const std::exception& e) {
        throw;
    }

}

void BasicRequestHandler::mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge)
{
	if(operation.type != toMerge.type){
		throw std::runtime_error("Cannot merge operation of diffrent types!");
	}
	switch(operation.type)
	{
		case SwtSequence::Operation::Read:
		{
			return;
		}
		break;		
		case SwtSequence::Operation::RMWbits:
		{
			operation.data[0] |= toMerge.data[0];
			operation.data[1] |= toMerge.data[1];
		}
		break;
		default:
			throw std::runtime_error("Unsupported operation to merge");
		break;
	}
}

SwtSequence::SwtOperation BasicRequestHandler::createSwtOperation(const WinCCRequest::Command& command)
{
	Board::ParameterInfo& parameter = m_board->at(command.name);
	if (parameter.regBlockSize != 1){
        throw std::runtime_error(parameter.name + ": regblock operations unsupported");
    }
	if(command.operation == WinCCRequest::Operation::Read){
		return SwtSequence::SwtOperation(SwtSequence::Operation::Read, parameter.baseAddress,{}, true);
	}

    if(parameter.isReadonly){
        	throw std::runtime_error(parameter.name + ": attempted WRITE on read-only parameter");
	}

    if(!command.value.has_value()){
        	throw std::runtime_error(parameter.name + ": no data for WRITE operation");
	}

	uint32_t rawValue = m_board->calculateRaw(parameter.name, command.value.value());

    if (parameter.bitLength == 32){
        return SwtSequence::SwtOperation(SwtSequence::Operation::Write, parameter.baseAddress, {rawValue});
	}

    return SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, parameter.baseAddress, 
                                    {SwtSequence::createANDMask(parameter.startBit, parameter.bitLength), rawValue});
}

std::string BasicRequestHandler::processMessageFromALF(std::string alfresponse)
{
    WinCCResponse response;

    try {
        AlfResponseParser alfMsg(alfresponse);

        if(!alfMsg.isSuccess()) {
            return handleErrorInALFResponse("ALF communication failed");
        }

        for (const auto& line : alfMsg) {
            switch(line.type)
            {
                case AlfResponseParser::Line::Type::ResponseToRead:
                    unpackReadResponse(line, response);
                break;
                case AlfResponseParser::Line::Type::ResponseToWrite:
                break;
            }
        }

    } catch (const std::exception& e) {
        return handleErrorInALFResponse(("Invalid response from ALF: " + string(e.what()));
    }

    return response.getContents();
}

void BasicRequestHandler::unpackReadResponse(const AlfResponseParser::Line& read, WinCCResponse& response)
{
    for(auto& parameterToHandle: m_registerTasks[read.frame.address])
    {
        double value = m_board->calculatePhysical(parameterToHandle.name, read.frame.data);
        if(parameterToHandle.toComapare.has_value()){
            
        }
        response.addParameter(parameterToHandle.name, {value});
        m_board->at(parameterToHandle.name).storeValue(value);
    }
}