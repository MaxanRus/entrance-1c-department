#pragma once

#include <boost/asio.hpp>
#include <string>
#include <tasks.hpp>
#include <deque>
#include <memory>

class Client;

namespace handlers {
using HandlerAcceptingClient = std::function<void(std::shared_ptr<Client>)>;
using HandlerReceivingMessage = std::function<void(std::string)>;
}  // namespace handlers

std::shared_ptr<Client> ConnectToServer(boost::asio::io_context&,
                                        const std::string host, int port);

class Client : public std::enable_shared_from_this<Client> {
 public:
  Client(boost::asio::io_context& io_context,
         boost::asio::ip::tcp::socket socket);

  void SetHandlerReceivingMessage(handlers::HandlerReceivingMessage handler);

  void PushTask(std::unique_ptr<Task>);

  void Start();
  void Stop();

 private:
  void DoTask();

  void AsyncReadHeader();
  void AsyncReadBody();
  void AsyncSend(const char* data, uint32_t length);
  void AsyncSendHeader();
  void AsyncSendBody();

  void OnReadHeader(const boost::system::error_code& error,
                    size_t bytes_transferred);
  void OnReadBody(const boost::system::error_code& error,
                  size_t bytes_transferred);
  void OnSendHeader(const boost::system::error_code& error,
                    size_t bytes_transferred);
  void OnSendBody(const boost::system::error_code& error,
                  size_t bytes_transferred);

  static constexpr uint32_t kSizeHeader = sizeof(uint32_t);
  static constexpr uint32_t kMaxSizeBody = 1024;

 private:
  boost::asio::io_context& io_context_;
  boost::asio::ip::tcp::socket socket_;

  std::deque<std::unique_ptr<Task>> task_queue_;

  uint32_t sending_size_;
  const char* sending_message_ = nullptr;

  uint32_t reading_size_;
  std::array<char, kMaxSizeBody> reading_message_;

  handlers::HandlerReceivingMessage handler_receiving_message_;

  friend class MessageData;

  friend class MessageString;
};
