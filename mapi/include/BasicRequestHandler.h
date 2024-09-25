#pragma once

#include<list>
#include<unordered_map>
#include<cstdint>
#include<memory>
#include"SwtSequence.h"

class Board;

class BasicRequestHandler
{
public:
struct ParameterToHandle{
  std::string name;
  std::optional<double> toComapare;
};

SwtSequence& processMessageFromWinCC(std::string);
std::string processMessageFromALF(std::string);

protected:
void resetExecutionData();
void mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge);
SwtSequence::SwtOperation createSwtOperation(const WinCCRequest::Command& command);

typedef std::list<ParameterToHandle> ToHandleList;

std::unordered_map<uint32_t, ToHandleList> m_registerTasks;
std::unordered_map<uint32_t, SwtSequence::SwtOperation> m_operations;
SwtSequence m_sequence;

std::shared_ptr<Board> m_board;
};
