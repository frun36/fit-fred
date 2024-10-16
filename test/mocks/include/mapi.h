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

    virtual ~Mapi();
    static void registerMapi(Fred* fred, string name);
};

struct Iterativemapi : Mapi {
    Iterativemapi();

    void newRequest(string request);
    void publishAnswer(string message);
    void publishError(string error);
};

struct Mapigroup : Mapi {
    Mapigroup(Fred* fred);

    void publishAnswer(string message);
    void publishError(string error);

    void newMapiGroupRequest(vector<pair<string, string>> requests);
    void newTopicGroupRequest(vector<pair<string, string>> requests);
};
