#include <easylogging++.h>

#include <boost/endian/conversion.hpp>
#include <chrono>
#include <iostream>
#include <server.hpp>

using namespace boost::asio;
using namespace std::placeholders;

std::shared_ptr<Client> ConnectToServer(boost::asio::io_context& io_context,
                                        const std::string host, int port) {
  ip::tcp::endpoint endpoint(ip::address::from_string(host), port);
  ip::tcp::socket socket(io_context);
  socket.connect(endpoint);

  return std::make_shared<Client>(io_context, std::move(socket));
}

Client::Client(io_context& io_context, ip::tcp::socket socket)
    : io_context_(io_context), socket_(std::move(socket)) {}

void Client::SetHandlerReceivingMessage(
    handlers::HandlerReceivingMessage handler) {
  handler_receiving_message_ = handler;
}

void Client::Start() { AsyncReadHeader(); }

/*
void Client::SendMessage(const std::string& message) {
  if (!writing_queue_.empty()) {
    writing_queue_.push_back(Message(message));
    return;
  }
  Message msg(message);
  writing_queue_.push_back(Message(message));
  AsyncSend();
}
*/

/*
void Client::SendMessage(char* data, uint32_t length) {
  bool current_writing = !writing_queue_.empty();

  if (length <= Message::kMaxSizeBody) {
    writing_queue_.push_back(Message(std::string(data, length)));
    if (current_writing) {
      return;
    }
    AsyncSend();
  }

  for (uint32_t i = 0; i < length; i += Message::kMaxSizeBody) {
    LOG(INFO) << data << " " << std::min(Message::kMaxSizeBody, length - i);
    writing_queue_.emplace_back(data + i,
                                std::min(Message::kMaxSizeBody, length - i));
    break;
  }
  if (current_writing) {
    return;
  }
  AsyncSend();
}
*/

void PushTask(std::unique_ptr<Task> task) {
  bool tasks_performered_ = !task_queue.empty();
  task_queue.push_back(std::move(task));

  if (!tasks_performered_) {
    DoTask();
  }
}

void Client::DoTask() {
  task_queue.front().Apply(*this);
  task_queue.pop_front();
  if (!task_queue.empty()) {
    DoTask();
  }
}

void Client::AsyncReadHeader() {
  socket_.async_read_some(
      reading_.GetLengthBuffer(),
      std::bind(&Client::OnReadHeader, shared_from_this(), _1, _2));
}

void Client::AsyncReadBody() {
  socket_.async_read_some(
      reading_.GetBodyBuffer(reading_.Length()),
      std::bind(&Client::OnReadBody, shared_from_this(), _1, _2));
}

void Client::AsyncSend(char* data, uint32_t length) {
  async_write(socket_, boost::asio::buffer(data, length),
              std::bind(&Client::OnSend, shared_from_this(), _1, _2));
  /*
auto& message = writing_queue_.front();
LOG(INFO) << "Sending " << (string_view)message;
if (message.IsMonolit()) {
  async_write(socket_, message.GetCurrentBuffer(),
              std::bind(&Client::OnSend, shared_from_this(), _1, _2));
} else {
  async_write(socket_, message.GetLengthBuffer(),
              std::bind(&Client::OnSendHeader, shared_from_this(), _1, _2));
}
*/
}

void Client::AsyncSendBody() {
  auto& message = writing_queue_.front();
  async_write(socket_, message.GetBodyBuffer(),
              std::bind(&Client::OnSend, shared_from_this(), _1, _2));
}

void LogError(const boost::system::error_code& error) {
  if (error == boost::asio::error::misc_errors::eof) {  // disconnect
    LOG(INFO) << "client disconnect";
    return;
  }
  if (error) {
    LOG(ERROR) << "Error: error while reading" << error.what();
    return;
  }
}

void Client::OnReadHeader(const boost::system::error_code& error,
                          size_t bytes_transferred) {
  LOG(INFO) << "Bytes read " << bytes_transferred;
  if (error) {
    LogError(error);
    return;
  }
  AsyncReadBody();
}

void Client::OnReadBody(const boost::system::error_code& error,
                        size_t bytes_transferred) {
  LOG(INFO) << "Bytes read " << bytes_transferred;
  if (error) {
    LogError(error);
    return;
  }
  handler_receiving_message_(std::move(reading_));
  AsyncReadHeader();
}

void Client::OnSendBody(const boost::system::error_code& error,
                    size_t bytes_transferred) {
  if (error) {
    LOG(ERROR) << "Error: error while send" << error.what();
  }
}

void Client::OnSendHeader(const boost::system::error_code& error,
                          size_t bytes_transferred) {
  if (error) {
    LOG(ERROR) << "Error: error while send" << error.what();
  }
  AsyncSendBody();
}

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

