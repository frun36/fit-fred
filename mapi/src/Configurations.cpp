#include "Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>

Configurations::Configurations(Fred* fred, const unordered_map<string, shared_ptr<Board>>& boards) : Mapigroup(fred)
{
    for (const auto& boardPair : boards) {
        const string& boardName = boardPair.first;
        if (boardName == "TCM")
            m_boardCofigurationServices[boardName] = make_unique<TcmConfigurations>(boards.at(boardName));
        else
            m_boardCofigurationServices[boardName] = make_unique<PmConfigurations>(boards.at(boardName));
    }
}

string Configurations::processInputMessage(string msg)
{
    if (!DatabaseInterface::isConnected())
        throw runtime_error("No DB connection");

    Utility::removeWhiteSpaces(msg);
    const string& name = msg;

    auto boardNamesData = DatabaseInterface::executeQuery("SELECT DISTINCT board_name FROM configurations WHERE configuration_name = '" + name + "';");
    if (boardNamesData.empty())
        throw runtime_error(name + ": configuration not found");

    vector<pair<string, string>> requests(boardNamesData.size());
    std::transform(boardNamesData.begin(), boardNamesData.end(), requests.begin(), [&name](const vector<MultiBase*>& entry) {
        if (!entry[0]->isString())
            throw runtime_error(name + ": invalid board name format in DB");

        return make_pair(entry[0]->getString(), name);
    });

    newMapiGroupRequest(requests);
    noRpcRequest = true;
    return "";
}

Configurations::BoardConfigurations::ConfigurationInfo Configurations::BoardConfigurations::getConfigurationInfo(const string& name)
{
    vector<SwtSequence::SwtOperation> sequenceVec;
    optional<int16_t> delayA = nullopt;
    optional<int16_t> delayC = nullopt;
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

    return ConfigurationInfo(processMessageFromWinCC(request.str()), delayA, delayC);
}

string Configurations::PmConfigurations::processInputMessage(string name)
{
    return getConfigurationInfo(name).seq.getSequence();
}

string Configurations::PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    if (parsedResponse.isError())
        returnError = true;
    return parsedResponse.getContents();
}

optional<SwtSequence> Configurations::TcmConfigurations::processDelayInput(optional<int16_t> delayA, optional<int16_t> delayC)
{
    if (!delayA.has_value() && !delayC.has_value())
        return nullopt;

    m_delayDifference = 0;

    string request;

    if (delayA.has_value()) {
        m_delayDifference = abs(delayA.value() - m_delayA.value_or(0));
        request += "DELAY_A,WRITE," + std::to_string(delayA.value()) + "\n";
        m_delayA = delayA;
    }

    if (delayC.has_value()) {
        int16_t cDelayDifference = abs(delayC.value() - m_delayC.value_or(0));
        if (cDelayDifference > m_delayDifference)
            m_delayDifference = cDelayDifference;
        request += "DELAY_C,WRITE," + std::to_string(delayC.value()) + "\n";
        m_delayC = delayC;
    }

    return processMessageFromWinCC(request);
}

string Configurations::TcmConfigurations::processInputMessage(string msg)
{
    optional<SwtSequence> delaySequence;
    switch (m_state) {
        case State::Idle:
            if (msg == ContinueMessage)
                throw runtime_error("TcmConfigurations: invalid state - use of internal message while idle");
            m_configurationName.emplace(msg);
            m_configurationInfo.emplace(getConfigurationInfo(msg));

            delaySequence = processDelayInput(m_configurationInfo->delayA, m_configurationInfo->delayC);
            if (delaySequence.has_value()) {
                m_state = State::ApplyingDelays;
                return delaySequence->getSequence();
            } else {
                m_state = State::ApplyingData;
                return m_configurationInfo->seq.getSequence();
            }
            break;

        case State::DelaysApplied:
            if (!m_configurationInfo.has_value())
                throw runtime_error("TcmConfigurations: invalid state - no configuration stored");

            m_state = State::ApplyingData;
            return m_configurationInfo->seq.getSequence();

        default:
            throw runtime_error("TcmConfigurations: invalid state in PIM");
    }
}

string Configurations::TcmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    string response = parsedResponse.getContents();
    switch (m_state) {
        case State::ApplyingDelays:
            if (parsedResponse.isError()) {
                reset();
                returnError = true;
                return "TCM configuration " + m_configurationName.value_or("<no name>") + " was not applied: delay change failed\n" + response;
            }

            m_delayResponse = response;
            m_state = State::DelaysApplied;
            usleep((m_delayDifference + 10) * 1000);
            newRequest(ContinueMessage);
            noReturn = true;
            return "";

        case State::ApplyingData:
            response = m_delayResponse.value_or("<no delay response>\n") + response;

            if (parsedResponse.isError()) {
                response = "TCM configuration " + m_configurationName.value_or("<no name>") + " was applied partially\n" + response;
                returnError = true;
            }

            reset();
            return response;

        default:
            throw runtime_error("TcmConfigurations: invalid state in POM");
    }
}