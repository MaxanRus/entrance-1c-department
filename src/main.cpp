#include <easylogging++.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <server.hpp>

INITIALIZE_EASYLOGGINGPP

int main() {
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

  Message m("0123");
  LOG(INFO) << m.Length();
  LOG(INFO) << (std::string_view)m;
  return 0;
}
