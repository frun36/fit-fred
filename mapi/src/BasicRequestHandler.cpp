#include"BasicRequestHandler.h"
#include"WinCCRequest.h"
#include"WinCCResponse.h"

SwtSequence& BasicRequestHandler::processMessageFromWinCC(std::string mess)
{
	resetExecutionData();
	WinCCRequest request(mess);

 	for (const auto& cmd : request.getCommands()) {

		SwtSequence::SwtOperation operation = createSwtOperation(cmd);
		Board::ParameterInfo& info = m_board->at(cmd.name);

		m_registerTasks[info.baseAddress].emplace_back(info,name, cmd.data);
		if(m_operations.find(info.baseAddress) != m_operations.end()){
			mergeOperation(m_operations.at(info.baseAddress), operation);
		}
		else{
			m_operatoions.emplace(info.baseAddress, operation);
		}	
	}

	if(request.isWrite()){
		for(auto& rmw: m_operations){
			m_sequence.addOperation(SwtSequence::Operation::Read, rmw.first, {}, true);
		}
	}

}

void BasicRequestHandler::mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge)
{
	if(operation.type != toMerge.type)
	{
		throw std::runtime_error("Cannot merge operation of diffrent types!");
	}
	switch(operation.type)
	{
		case SwtSequence::Operation::Read:
		{
			return;
		}
		break;		
		case SwtSequence::Operation::RMWbits
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
	if (parameter.regBlockSize != 1)
    	{
        	throw std::runtime_error(info.name + ": regblock operations unsupported");
    	}
	if(command.operation == WinCCRequest::Operation::Read)
	{
		return SwtSequence::SwtOperation(SwtSequence::Operation::Read, parameter.baseAddress,{}, true);
	}

	// WRITE operation
    	if(parameter.isReadonly)
	{
        	throw std::runtime_error(parmeter.name + ": attempted WRITE on read-only parameter");
	}

    	if(!command.data.has_value())
	{
        	throw std::runtime_error(parameter.name + ": no data for WRITE operation");
	}

	uint32_t rawValue = m_board->at(paramater.name).calculateRawValue(data.value());

    if (parameter.bitLength == 32)
	{
        return SwtSequence::SwtOperation(SwtSequence::Operation::Write, parameter.baseAddress, {rawValue});
	}

    // needs RMW
    std::array<uint32_t, 2> masks;
    SwtSequence::createMask(parameter.startBit, parameter.bitLength, rawValue >> parameter.startBit, masks.data());
    return SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, parameter.baseAddress, masks);
}

