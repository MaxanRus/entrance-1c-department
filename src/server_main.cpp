#include <easylogging++.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <server.hpp>

INITIALIZE_EASYLOGGINGPP

namespace bio = boost::iostreams;
using File = bio::mapped_file;

void GetMessage(std::weak_ptr<Client> client, std::string message) {
  LOG(INFO) << "Server recv message length: " << message.length();

  namespace bio = boost::iostreams;
  File file;
  file.open(std::string(message.substr(4, message.length() - 4)),
            boost::iostreams::mapped_file::readonly);

  client.lock()->PushTask(std::make_unique<MessageString>(std::to_string(file.size())));
  client.lock()->PushTask(std::make_unique<MessageData>(file.const_data(), file.size()));
  client.lock()->PushTask(std::make_unique<Function>([file](Client& client) mutable { file.close(); }));
}

void NewClient(std::shared_ptr<Client> client) {
  using namespace std::placeholders;
  LOG(INFO) << "Connect new client";
  client->SetHandlerReceivingMessage([client](auto&& message) {
    return GetMessage(client,
                      std::forward<decltype(message)>(
                          message));
  });
  client->Start();
}

int main(int argc, char* argv[]) {
  int port = 6666;
  if (argc != 2) {
    LOG(INFO) << "Server start on default port " << port;
  } else {
    port = std::atoi(argv[1]);
    LOG(INFO) << "Server start on port" << port;
  }
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

  Server server("0.0.0.0", 6666);
  server.SetHandlerAcceptionClient(&NewClient);
  server.Start();
}
