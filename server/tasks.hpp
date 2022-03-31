#pragma once

#include <string>
#include <functional>

class Client;

struct Task {
  virtual void Apply(Client&) = 0;
  virtual ~Task() = 0;
};

struct MessageData : Task {
  MessageData(const char* data, uint32_t length) : data(data), length(length) {}

  void Apply(Client& client) override;

  const char* data;
  uint32_t length;
};

struct MessageString : Task {
  explicit MessageString(std::string data) : data(std::move(data)) {}

  void Apply(Client& client) override;

  std::string data;
};

struct Function : Task {
  Function(std::function<void(Client&)> function) : function(std::move(function)) {}

  void Apply(Client& client) override;

  std::function<void(Client&)> function;
};
