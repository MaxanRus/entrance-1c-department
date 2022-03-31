#include <easylogging++.h>

#include <client.hpp>
#include <server.hpp>
#include <memory>

using namespace boost::asio;
using namespace std::placeholders;

Server::Server(const std::string& host, int port)
    : acceptor_(io_context_,
                ip::tcp::endpoint(ip::address::from_string(host), port)) {}

void Server::Start() {
  WaitNewClient();
  io_context_.run();
}

void Server::SetHandlerAcceptionClient(
    handlers::HandlerAcceptingClient handler) {
  handler_accepting_client_ = std::move(handler);
}

void Server::WaitNewClient() {
  acceptor_.async_accept(std::bind(&Server::AcceptClient, this, _1, _2));
}

void Server::AcceptClient(const boost::system::error_code& error,
                          ip::tcp::socket socket) {
  WaitNewClient();

  handler_accepting_client_(
      std::make_shared<Client>(io_context_, std::move(socket)));
}
