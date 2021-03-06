#include <client.hpp>
#include <easylogging++.h>

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

void Client::PushTask(std::unique_ptr<Task> task) {
  bool tasks_performered_ = !task_queue_.empty();
  task_queue_.push_back(std::move(task));

  if (!tasks_performered_) {
    DoTask();
  }
}

void Client::DoTask() {
  task_queue_.front()->Apply(*this);
}

void Client::AsyncReadHeader() {
  socket_.async_read_some(
      boost::asio::buffer(&reading_size_, kSizeHeader),
      std::bind(&Client::OnReadHeader, shared_from_this(), _1, _2));
}

void Client::AsyncReadBody() {
  socket_.async_read_some(
      boost::asio::buffer(reading_message_.data(), std::min(kMaxSizeBody, reading_size_)),
      std::bind(&Client::OnReadBody, shared_from_this(), _1, _2));
}

void Client::AsyncSend(const char* data, uint32_t length) {
  sending_message_ = data;
  sending_size_ = length;
  AsyncSendHeader();
}

void Client::AsyncSendHeader() {
  async_write(socket_, boost::asio::buffer(&sending_size_, kSizeHeader),
              std::bind(&Client::OnSendHeader, shared_from_this(), _1, _2));
}

void Client::AsyncSendBody() {
  async_write(socket_, boost::asio::buffer(sending_message_, std::min(kMaxSizeBody, sending_size_)),
              std::bind(&Client::OnSendBody, shared_from_this(), _1, _2));
}

void LogError(const boost::system::error_code& error) {
  if (error == boost::asio::error::misc_errors::eof || error == boost::asio::error::bad_descriptor) {  // disconnect
    LOG(INFO) << "Disconnect";
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
  handler_receiving_message_(std::string(reading_message_.data(), std::min(kMaxSizeBody, reading_size_)));
  if (reading_size_ > kMaxSizeBody) {
    reading_size_ -= kMaxSizeBody;
    AsyncReadBody();
  } else {
    AsyncReadHeader();
  }
}

void Client::OnSendHeader(const boost::system::error_code& error,
                          size_t bytes_transferred) {
  if (error) {
    LOG(ERROR) << "Error: error while send" << error.what();
  }
  AsyncSendBody();
}

void Client::OnSendBody(const boost::system::error_code& error,
                        size_t bytes_transferred) {
  if (error) {
    LOG(ERROR) << "Error: error while send" << error.what();
  }
  LOG(INFO) << "Transferred: " << bytes_transferred;
  if (sending_size_ > kMaxSizeBody) {
    sending_message_ += kMaxSizeBody;
    sending_size_ -= kMaxSizeBody;
    AsyncSendBody();
  } else {
    task_queue_.pop_front();
    if (!task_queue_.empty()) {
      DoTask();
    }
  }
}

void Client::Stop() {
  socket_.close();
}
