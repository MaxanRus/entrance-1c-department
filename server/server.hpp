#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <deque>
#include <memory>
#include <variant>
#include <array>

class Client;

class Server;

namespace handlers {
using HandlerAcceptingClient = std::function<void(std::shared_ptr<Client>)>;
using HandlerReceivingMessage = std::function<void(std::string)>;
}  // namespace handlers

class Server {
 public:
  Server(const std::string& host, int port);

  void Start();
  void SetHandlerAcceptionClient(handlers::HandlerAcceptingClient handler);

 private:
  void WaitNewClient();
  void AcceptClient(const boost::system::error_code& error,
                    boost::asio::ip::tcp::socket socket);

 private:
  boost::asio::io_context io_context_;
  boost::asio::ip::tcp::acceptor acceptor_;

  handlers::HandlerAcceptingClient handler_accepting_client_;
};
