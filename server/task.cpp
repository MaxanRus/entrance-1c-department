#include <tasks.hpp>
#include <client.hpp>

void MessageData::Apply(Client& client) {
  client.AsyncSend(data, length);
}

void MessageString::Apply(Client& client) {
  client.AsyncSend(data.c_str(), data.length());
}

void Function::Apply(Client& client) { function(client); }

Task::~Task() {}
