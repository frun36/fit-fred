#pragma once

#include <string>
#include <vector>
#include <utility>

using namespace std;

struct Fred {
    string Name() const { return "FRED"; }
};

struct Mapi {
    Mapi();

    Fred* fred;
    bool returnError;
    bool noReturn;
    bool noRpcRequest;

    virtual string processInputMessage(string msg) = 0;
    virtual string processOutputMessage(string msg) = 0;

    virtual ~Mapi() = default;
    static void registerMapi(Fred* fred, string name);
};

struct Iterativemapi : Mapi {
    Iterativemapi() = default;

    void newRequest(string request);
    void publishAnswer(string message);
    void publishError(string error);
};

struct Mapigroup : Mapi {
    Mapigroup() = default;

    void publishAnswer(string message);
    void publishError(string error);

    void newMapiGroupRequest(vector<pair<string, string>> requests);
    void newTopicGroupRequest(vector<pair<string, string>> requests);
};

struct IndefiniteMapi : Mapi {
    IndefiniteMapi() = default;

    void publishAnswer(string message);
    void publishError(string error);

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    string executeAlfSequence(string seq);
    string waitForRequest(bool& running);

    virtual void processExecution() = 0;
};