#pragma once

#include<list>
#include<unordered_map>
#include<cstdint>
#include<memory>
#include"SwtSequence.h"
#include"AlfResponseParser.h"
#include"WinCCRequest.h"
#include"WinCCResponse.h"

class Board;

class BasicRequestHandler
{
public:
BasicRequestHandler(std::shared_ptr<Board> board): m_board(board)
{

}

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
  std::string name;
  std::optional<double> toCompare;
};

void resetExecutionData();
void mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge);
SwtSequence::SwtOperation createSwtOperation(const WinCCRequest::Command& command);

void unpackReadResponse(const AlfResponseParser::Line& read, WinCCResponse& response, std::list<ErrorReport>& report);

virtual std::string handleErrorInALFResponse(std::string);

typedef std::list<ParameterToHandle> ToHandleList;

std::unordered_map<uint32_t, ToHandleList> m_registerTasks;
std::unordered_map<uint32_t, SwtSequence::SwtOperation> m_operations;

std::shared_ptr<Board> m_board;
};