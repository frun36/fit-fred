#pragma once

#include<list>
#include<unordered_map>
#include<cstdint>
#include<memory>
#include"SwtSequence.h"
#include"AlfResponseParser.h"
#include"WinCCRequest.h"
#include"WinCCResponse.h"
#include"Board.h"

class BasicRequestHandler
{
public:
BasicRequestHandler(std::shared_ptr<Board> board): m_board(board)
{

}
virtual ~BasicRequestHandler() = default;

struct ErrorReport{
  ErrorReport() = default;
  ErrorReport(const std::string& param, const std::string& message):
  parameterName(param), mess(message)
  {}

  std::string parameterName;
  std::string mess;

  std::string what(){
    return "ERROR - " + parameterName + ": " + mess; 
  }
};

SwtSequence processMessageFromWinCC(std::string);
std::pair<WinCCResponse,std::list<ErrorReport>>  processMessageFromALF(std::string);

protected:
struct ParameterToHandle{
  ParameterToHandle(const std::string& name_, const std::optional<double>& toCompare_): name(name_), toCompare(toCompare_) {}
  ParameterToHandle(const std::string& name_, std::optional<double>&& toCompare_): name(name_), toCompare(toCompare_) {}
  std::string name;
  std::optional<double> toCompare;
};

void resetExecutionData();
void mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge);
SwtSequence::SwtOperation createSwtOperation(const WinCCRequest::Command& command);

void unpackReadResponse(const AlfResponseParser::Line& read, WinCCResponse& response, std::list<ErrorReport>& report);

typedef std::list<ParameterToHandle> ToHandleList;

std::unordered_map<uint32_t, ToHandleList> m_registerTasks;
std::unordered_map<uint32_t, SwtSequence::SwtOperation> m_operations;

std::shared_ptr<Board> m_board;
};
