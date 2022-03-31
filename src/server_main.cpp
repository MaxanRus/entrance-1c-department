#include <easylogging++.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <server.hpp>

INITIALIZE_EASYLOGGINGPP

namespace bio = boost::iostreams;
using File = bio::mapped_file;

File file;

void GetMessage(std::weak_ptr<Client> client, Message s) {
  LOG(INFO) << "Server recv message: " << (std::string_view)s;

  namespace bio = boost::iostreams;
  std::string_view message(s);
  file.open(std::string(message.substr(4, message.length() - 4)),
                        boost::iostreams::mapped_file::readonly);

  std::string test(file.const_data(), file.const_data() + file.size());
  LOG(INFO) << test;

  // client.lock()->SendMessage(std::to_string(test.length()));
  client.lock()->SendMessage(const_cast<char*>(file.const_data()), file.size());
  client.lock()->SendMessage(test);
}

void NewClient(std::shared_ptr<Client> c) {
  using namespace std::placeholders;
  LOG(INFO) << "Connect new client";
  c->SetHandlerReceivingMessage(std::bind(&GetMessage, c, _1));
  c->Start();
}

int main() {
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

  Server server("0.0.0.0", 6666);
  server.SetHandlerAcceptionClient(&NewClient);
  server.Start();
}
