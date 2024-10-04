#include "Configurations.h"

Configurations::Configurations(Fred* fred, const unordered_map<string, Board>& boards)
{
    for (const auto& boardPair : boards) {
        const string& boardName = boardPair.first;
        if (boardName == "TCM")
            m_boardCofigurationServices[boardName] = make_unique<TcmConfigurations>();
        else
            m_boardCofigurationServices[boardName] = make_unique<PmConfigurations>();
    }

    auto names = DatabaseInterface::executeQuery("SELECT UNIQUE configuration_name FROM configurations");

    for (const auto& wrappedConfigurationName : names) {
        const string& configurationName = wrappedConfigurationName[0]->getString();

        auto boardNames = DatabaseInterface::executeQuery("SELECT UNIQUE board_name FROM configurations WHERE configuration_name = " + configurationName);

        for (const auto& wrappedBoardName : boardNames) {
            const string& boardName = wrappedBoardName[0]->getString();
            m_boardsInConfigurations[configurationName].push_back(boardName);

            auto configurationInfo =
                DatabaseInterface::executeQuery("SELECT parameter_name, parameter_value FROM parameters p JOIN configurations c ON p.parameter_id = c.parameter_id WHERE configuration_name = " + configurationName + " AND board_name = " + boardName);

            m_boardCofigurationServices[boardName]->registerConfiguration(configurationName, configurationInfo);
        }
    }
}

void Configurations::BoardConfigurations::registerConfiguration(const string& name, vector<vector<MultiBase*>> dbData)
{
    vector<SwtSequence::SwtOperation> sequenceVec;
    optional<int16_t> delayA = nullopt;
    optional<int16_t> delayC = nullopt;

    for (const auto& row : dbData) {
        if (row.size() != 2 || !row[0]->isString() || !row[1]->isDouble())
            throw runtime_error("Invalid CONFIGURATIONS data row");

        string parameterName = row[0]->getString();
        double parameterValue = row[1]->getDouble();
        if (parameterName == "DELAY_A")
            delayA = parameterValue;
        else if (parameterName == "DELAY_C")
            delayC = parameterValue;
        else
            sequenceVec.push_back(createSwtOperation(WinCCRequest::Command(parameterName, WinCCRequest::Operation::Write, parameterValue)));
    }

    getKnownConfigurations().insert_or_assign(name, ConfigurationInfo(SwtSequence(sequenceVec), delayA, delayC));
}

string Configurations::PmConfigurations::processInputMessage(string msg)
{
    Utility::removeWhiteSpaces(msg);
    if (m_knownConfigurations.count(msg) == 0)
        throw std::runtime_error(msg + ": no such configuration found");

    return m_knownConfigurations.at(msg).seq.getSequence();
}

string Configurations::PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    if (parsedResponse.errors.size() != 0) {
        returnError = true;
        std::stringstream error;
        for (auto& report : parsedResponse.errors)
            error << report.what() << '\n';
        error << parsedResponse.response.getContents();
        return error.str();
    }
    return parsedResponse.response.getContents();
}

optional<SwtSequence> Configurations::TcmConfigurations::processDelayInput(optional<int16_t> delayA, optional<int16_t> delayC)
{
    if (!delayA.has_value() && !delayC.has_value())
        return nullopt;

    string request;

    if (delayA.has_value()) {
        m_delayDifference = abs(delayA.value() - m_currDelayA.value_or(0));
        request += "DELAY_A,WRITE," + std::to_string(delayA.value()) + "\n";
        m_currDelayA = delayA;
    }

    if (delayC.has_value()) {
        int16_t cDelayDifference = abs(delayC.value() - m_currDelayC.value_or(0));
        if (cDelayDifference > m_delayDifference)
            m_delayDifference = cDelayDifference;
        request += "DELAY_C,WRITE," + std::to_string(delayC.value()) + "\n";
        m_currDelayC = delayC;
    }

    return processMessageFromWinCC(request);
}

string Configurations::TcmConfigurations::processInputMessage(string msg)
{
    if (m_currState != State::Idle && strncmp(msg.c_str(), INTERNAL_PREFIX, strlen(INTERNAL_PREFIX)) != 0)
        throw std::runtime_error(msg + ": another configuration is already in progress");

    optional<SwtSequence> delaySequence;
    switch (m_currState) {
        case State::Idle:
            Utility::removeWhiteSpaces(msg);
            if (m_knownConfigurations.count(msg) == 0)
                throw std::runtime_error(msg + ": no such configuration found");
            m_currCfg = &m_knownConfigurations[msg];
            delaySequence = processDelayInput(m_currCfg->delayA, m_currCfg->delayC);
            if (delaySequence.has_value()) {
                m_currState = State::ApplyDelays;
                return delaySequence->getSequence();
            }
            break;

        case State::ApplyDelays:
        case State::ApplyData:
            return m_currCfg->seq.getSequence();
    }
}

string Configurations::TcmConfigurations::processOutputMessage(string msg)
{
    auto response = processMessageFromALF(msg);

    switch (m_currState) {
        case State::ApplyDelays:
    }

    // parse alf response
    // generate response
    // if delays - send request for data
    // if data - ok
    // reset and counters?
}