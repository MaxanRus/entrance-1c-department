#include <easylogging++.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <chrono>
#include <server.hpp>
#include <thread>

INITIALIZE_EASYLOGGINGPP

namespace bio = boost::iostreams;
namespace bfs = boost::filesystem;

bio::mapped_file file;

bio::mapped_file CreateResultFile(const std::string& path, size_t new_length) {
  bio::mapped_file file;
  bio::mapped_file_params params;

  params.path = path;
  params.flags = bio::mapped_file::mapmode::readwrite;
  params.new_file_size = new_length;

  file.open(params);
  return file;
}

using File = bio::mapped_file;

void PrintToFile(std::weak_ptr<Client> client, File file, size_t offset, Message s) {
  using namespace std::placeholders;
  std::string_view msg(s);

  std::copy(msg.begin(), msg.end(), file.data() + offset);
  client.lock()->SetHandlerReceivingMessage(
      std::bind(&PrintToFile, client, file, offset + msg.length(), _1));
}

void GetLengthFile(std::weak_ptr<Client> client, Message s) {
  using namespace std::placeholders;
  LOG(INFO) << "Client receiving message with a length " << s.Length();
  std::string_view msg(s);
  LOG(INFO) << msg;

  file = CreateResultFile("recv.txt", std::atoi(msg.data()));

  client.lock()->SetHandlerReceivingMessage(
      std::bind(&PrintToFile, client, file, 0, _1));
}

int main() {
  using namespace std::placeholders;
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

  boost::asio::io_context io;

  auto client = ConnectToServer(io, "127.0.0.1", 6666);

  client->SetHandlerReceivingMessage(std::bind(&GetLengthFile, client, _1));
  client->Start();

  client->SendMessage("get test.txt");

  io.run();
}
