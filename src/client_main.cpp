#include <easylogging++.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <chrono>
#include <server.hpp>
#include <thread>
#include <client.hpp>

INITIALIZE_EASYLOGGINGPP

namespace bio = boost::iostreams;
using File = bio::mapped_file;

bio::mapped_file CreateResultFileAndTranslateToMemory(const std::string& path, size_t new_length) {
  bio::mapped_file file;
  bio::mapped_file_params params;

  std::ofstream ofstream(path); // create file
  ofstream.close();

  params.path = path;
  params.flags = bio::mapped_file::mapmode::readwrite;
  params.new_file_size = new_length;

  file.open(params);
  return file;
}

void PrintToFile(std::weak_ptr<Client> client, File file, size_t offset, std::string msg) {
  using namespace std::placeholders;

  std::copy(msg.begin(), msg.end(), file.data() + offset);
  if (offset + msg.length() == file.size()) {
    client.lock()->PushTask(std::make_unique<Function>([](Client& client) { client.Stop(); }));
    file.close();
  } else {
    client.lock()->SetHandlerReceivingMessage(
        [client, file, new_offset = offset + msg.length()](auto&& message) {
          return PrintToFile(client,
                             file,
                             new_offset,
                             std::forward<decltype(message)>(message));
        });
  }
}

void GetLengthFile(std::weak_ptr<Client> client, const std::string& result_filename, std::string msg) {
  using namespace std::placeholders;
  LOG(INFO) << "Client receiving message with a length " << msg.length();

  File file = CreateResultFileAndTranslateToMemory(result_filename, std::atoi(msg.data()));

  client.lock()->SetHandlerReceivingMessage(
      [client, file](auto&& message) {
        return PrintToFile(client,
                           file,
                           0,
                           std::forward<decltype(message)>(message));
      });
}

int main(int argc, char* argv[]) {
  std::string host = "127.0.0.1";
  int port = 6666;
  std::string filename;
  if (argc == 1) {
    LOG(ERROR)
        << "Please run application [./Client file_name] or [./Client host file_name] or [./Client host port file_name]";
    return 0;
  } else if (argc == 2) {
    filename = argv[1];
  } else if (argc == 3) {
    host = argv[1];
    filename = argv[2];
  } else if (argc == 4) {
    host = argv[1];
    port = std::atoi(argv[2]);
    filename = argv[3];
  }

  using namespace std::placeholders;
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

  boost::asio::io_context io;

  auto client = ConnectToServer(io, host, port);

  client->SetHandlerReceivingMessage([client, &filename](auto&& message) {
    return GetLengthFile(client,
                         "download_" + filename,
                         std::forward<decltype(message)>(message));
  });
  client->Start();

  client->PushTask(std::make_unique<MessageString>("get " + filename));

  io.run();
}
