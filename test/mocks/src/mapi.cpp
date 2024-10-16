#include "../include/mapi.h"

Mapi::Mapi() : returnError(false), noReturn(false), noRpcRequest(false) {}

Mapi::~Mapi() = default;

void Mapi::registerMapi(Fred* fred, string name) {}

Iterativemapi::Iterativemapi() = default;

void Iterativemapi::newRequest(string request) {}

void Iterativemapi::publishAnswer(string message) {}

void Iterativemapi::publishError(string error) {}

Mapigroup::Mapigroup() = default;

void Mapigroup::publishAnswer(string message) {}

void Mapigroup::publishError(string error) {}

void Mapigroup::newMapiGroupRequest(vector<pair<string, string>> requests) {}

void Mapigroup::newTopicGroupRequest(vector<pair<string, string>> requests) {}
