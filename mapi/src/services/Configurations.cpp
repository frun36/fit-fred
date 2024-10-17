#include "services/Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>

Configurations::Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards) : m_fredName(fredName)
{
    for (auto [name, board] : boards) {
        if (name == "TCM")
            m_boardCofigurationServices[name] = make_unique<TcmConfigurations>(board);
        else
            m_boardCofigurationServices[name] = make_unique<PmConfigurations>(board);
    }
}

string Configurations::processInputMessage(string msg)
{
    if (!DatabaseInterface::isConnected())
        throw runtime_error("No DB connection");

    Utility::removeWhiteSpaces(msg);
    const string& configurationName = msg;

    auto boardNamesData = DatabaseInterface::executeQuery("SELECT DISTINCT board_name FROM configurations WHERE configuration_name = '" + configurationName + "';");
    if (boardNamesData.empty())
        throw runtime_error(configurationName + ": configuration not found");

    vector<pair<string, string>> requests(boardNamesData.size());
    std::transform(boardNamesData.begin(), boardNamesData.end(), requests.begin(), [&configurationName, this](const vector<MultiBase*>& entry) {
        if (!entry[0]->isString())
            throw runtime_error(configurationName + ": invalid board name format in DB");

        string boardName = entry[0]->getString();
        string serviceName = m_fredName;
        if (boardName == "TCM")
            serviceName += "TCM/TCM0/";
        else if (boardName.find("PM") != string::npos)
            serviceName += "PM/" + boardName + "/";
        serviceName += "_INTERNAL_CONFIGURATIONS";

        return make_pair(serviceName, configurationName);
    });

    newMapiGroupRequest(requests);
    noRpcRequest = true;
    return "";
}

Configurations::BoardConfigurations::ConfigurationInfo Configurations::BoardConfigurations::getConfigurationInfo(const string& name)
{
    vector<SwtSequence::SwtOperation> sequenceVec;
    optional<double> delayA = nullopt;
    optional<double> delayC = nullopt;
    auto dbData = DatabaseInterface::executeQuery("SELECT parameter_name, parameter_value FROM parameters p JOIN configurations c ON p.parameter_id = c.parameter_id WHERE configuration_name = '" + name + "' AND board_name = '" + m_board->getName() + "';");
    stringstream request;
    for (const auto& row : dbData) {
        if (row.size() != 2 || !row[0]->isString() || !row[1]->isDouble())
            throw runtime_error(name + ": invalid CONFIGURATIONS data row");

        string parameterName = row[0]->getString();
        double parameterValue = row[1]->getDouble();
        if (parameterName == "DELAY_A")
            delayA = parameterValue;
        else if (parameterName == "DELAY_C")
            delayC = parameterValue;
        else
            request << parameterName << ",WRITE," << parameterValue << "\n";
    }

    return ConfigurationInfo(request.str(), delayA, delayC);
}

string Configurations::PmConfigurations::processInputMessage(string name)
{
    return processMessageFromWinCC(getConfigurationInfo(name).req).getSequence();
}

string Configurations::PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    if (parsedResponse.isError())
        returnError = true;
    return parsedResponse.getContents();
}

optional<SwtSequence> Configurations::TcmConfigurations::processDelayInput(optional<double> delayA, optional<double> delayC)
{
    if (!delayA.has_value() && !delayC.has_value())
        return nullopt;

    m_delayDifference = 0;

    string request;

    if (delayA.has_value()) {
        m_delayDifference = abs(delayA.value() - getDelayA().value_or(0));
        if (m_delayDifference != 0)
            request += "DELAY_A,WRITE," + std::to_string(delayA.value()) + "\n";
    }

    if (delayC.has_value()) {
        int16_t cDelayDifference = abs(delayC.value() - getDelayC().value_or(0));
        if (cDelayDifference > m_delayDifference)
            m_delayDifference = cDelayDifference;
        if (cDelayDifference != 0)
            request += "DELAY_C,WRITE," + std::to_string(delayC.value()) + "\n";
    }

    return processMessageFromWinCC(request);
}

string Configurations::TcmConfigurations::handleConfigurationStart(const string& msg)
{
    optional<SwtSequence> delaySequence;
    if (msg == CONTINUE_MESSAGE)
        throw runtime_error("TcmConfigurations: invalid state - use of internal message while idle");
    m_configurationName.emplace(msg);
    m_configurationInfo.emplace(getConfigurationInfo(msg));
    delaySequence = processDelayInput(m_configurationInfo->delayA, m_configurationInfo->delayC);
    if (delaySequence.has_value()) {
        m_state = State::ApplyingDelays;
        return delaySequence->getSequence();
    } else {
        m_state = State::ApplyingData;
        return processMessageFromWinCC(m_configurationInfo->req).getSequence();
    }
}

string Configurations::TcmConfigurations::handleConfigurationContinuation(const string& msg)
{
    if (msg != CONTINUE_MESSAGE)
        throw runtime_error("TcmConfigurations: previous configuration still in progress");

    if (!m_configurationInfo.has_value())
        throw runtime_error("TcmConfigurations: invalid state - no configuration stored");

    m_state = State::ApplyingData;
    return processMessageFromWinCC(m_configurationInfo->req).getSequence();
}

string Configurations::TcmConfigurations::handleDelayResponse(const string& msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    string response = parsedResponse.getContents();

    if (parsedResponse.isError()) {
        response = "TCM configuration " + m_configurationName.value_or("<no name>") + " was not applied: delay change failed\n" + response;
        reset();
        returnError = true;
        return response;
    }

    m_delayResponse = response;
    m_state = State::DelaysApplied;
    // Time unit needs to be considered after electronics analysis
    usleep((static_cast<useconds_t>(m_delayDifference) + 10) * 1000);
    newRequest(CONTINUE_MESSAGE);
    noReturn = true;
    return "";
}

string Configurations::TcmConfigurations::handleDataResponse(const string& msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    string response = parsedResponse.getContents();
    response = m_delayResponse.value_or("") + response;

    if (parsedResponse.isError()) {
        response = "TCM configuration " + m_configurationName.value_or("<no name>") + (m_delayResponse.has_value() ? " was applied partially\n" : " was not applied\n") + response;
        returnError = true;
    }

    reset();
    return response;
}

string Configurations::TcmConfigurations::processInputMessage(string msg)
{
    switch (m_state) {
        case State::Idle:
            return handleConfigurationStart(msg);

        case State::DelaysApplied:
            return handleConfigurationContinuation(msg);

        default:
            throw runtime_error("TcmConfigurations: invalid state in PIM");
    }
}

string Configurations::TcmConfigurations::processOutputMessage(string msg)
{
    switch (m_state) {
        case State::ApplyingDelays:
            return handleDelayResponse(msg);

        case State::ApplyingData:
            return handleDataResponse(msg);

        default:
            throw runtime_error("TcmConfigurations: invalid state in POM");
    }
}