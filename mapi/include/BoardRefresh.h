#pragma once
#include<indefinitemapi.h>
#include<list>
#include<unordered_map>
#include<chrono>

#include"SwtSequence.h"

class BoardRefresh: public IndefiniteMapi
{
public:
    BoardRefresh(std::list<std::string> parameters);
private:
    void processExecution();

    SwtSequence m_request;
    const std::chrono::milliseconds m_sleepPeriod{1000};
};