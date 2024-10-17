#include "../include/mapi.h"

Mapi::Mapi() : returnError(false), noReturn(false), noRpcRequest(false) {}

Mapi::~Mapi() = default;

void Mapi::registerMapi([[maybe_unused]] Fred* fred, [[maybe_unused]] string name) {}

Iterativemapi::Iterativemapi() = default;

void Iterativemapi::newRequest([[maybe_unused]] string request) {}

void Iterativemapi::publishAnswer([[maybe_unused]] string message) {}

void Iterativemapi::publishError([[maybe_unused]] string error) {}

Mapigroup::Mapigroup() = default;

void Mapigroup::publishAnswer([[maybe_unused]] string message) {}

void Mapigroup::publishError([[maybe_unused]] string error) {}

void Mapigroup::newMapiGroupRequest([[maybe_unused]] vector<pair<string, string>> requests) {}

void Mapigroup::newTopicGroupRequest([[maybe_unused]] vector<pair<string, string>> requests) {}
