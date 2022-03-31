#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <deque>
#include <memory>
#include <variant>

class Client;
class Server;

/*
class Message {
 public:
  Message() {
    auto q = std::make_unique<char[]>(kSizeHeader + kMaxSizeBody);
    *reinterpret_cast<uint32_t*>(q.get()) = 0;
    data_ = std::move(q);
  }

  explicit Message(const std::string& text) {
    auto q = std::make_unique<char[]>(kSizeHeader + kMaxSizeBody);
    *reinterpret_cast<uint32_t*>(q.get()) = (uint32_t)text.length();
    std::copy(text.begin(), text.end(), ((char*)q.get()) + kSizeHeader);
    data_ = std::move(q);
  }

  Message(char* data, uint32_t length) {
    data_ = NotMonolithData{data, length};
  }

  Message(Message&& another) {
    data_ = std::move(another.data_);
    // another.data_ = NotMonolithData{nullptr, 0};
    auto q = std::make_unique<char[]>(kSizeHeader + kMaxSizeBody);
    *reinterpret_cast<uint32_t*>(q.get()) = 0;
    another.data_ = std::move(q);
  }

  Message& operator=(const Message& another) = delete;

  Message& operator=(Message&& another) {
    std::swap(another.data_, data_);
    return *this;
  }

  explicit operator const std::string_view() {
    return std::string_view(GetData(), Length());
  }

  explicit operator std::string() {
    return std::string(this->operator const std::string_view());
  }

  boost::asio::mutable_buffer GetCurrentBuffer() {
    return boost::asio::buffer(GetLength(), kSizeHeader + Length());
  }

  boost::asio::mutable_buffer GetLengthBuffer() {
    return boost::asio::buffer(GetLength(), kSizeHeader);
  }

  boost::asio::mutable_buffer GetBodyBuffer() {
    return boost::asio::buffer(GetData(), Length());
  }

  boost::asio::mutable_buffer GetBodyBuffer(size_t length) {
    return boost::asio::buffer(GetData(), length);
  }

  char* GetLength() {
    if (data_.index() == 0) {
      return reinterpret_cast<char*>(std::get<NotMonolithData>(data_).length);
    } else {
      return reinterpret_cast<char*>(std::get<MonolithData>(data_).get());
    }
  }

  char* GetData() {
    if (data_.index() == 0) {
      return std::get<NotMonolithData>(data_).data;
    } else {
      return reinterpret_cast<char*>(std::get<MonolithData>(data_).get()) +
             kSizeHeader;
    }
  }

  bool IsMonolit() const { return data_.index() == 1; }

  size_t Length() {
    LOG(GetLength());
    return static_cast<size_t>(*reinterpret_cast<uint32_t*>(GetLength()));
  }

  static constexpr uint32_t kSizeHeader = sizeof(uint32_t);
  static constexpr uint32_t kMaxSizeBody = 1024;
 private:
  struct NotMonolithData {
    char* data;
    uint32_t length;
  };

  using MonolithData = std::unique_ptr<char[]>;

  std::variant<NotMonolithData, MonolithData> data_;
};
*/

struct Task {
  virtual void Apply(Client&);
  virtual ~Task() = 0;
};

struct MessageData : Task {
  virtual void Apply(Client& client) { client.AsyncSend(data, length); }

  char* data;
  uint32_t length;
};

struct MessageString : Task {
  virtual void Apply(Client& client) {
    client.AsyncSend(data.c_str(), data.length());
  }

  std::string data;
};

struct Function : Task {
  virtual void Apply(Client& client) { function(client); }

  std::function<void(Client&)> function;
};

namespace handlers {
  using HandlerAcceptingClient = std::function<void(std::shared_ptr<Client>)>;
  using HandlerReceivingMessage = std::function<void(Message)>;
}  // namespace handlers

std::shared_ptr<Client> ConnectToServer(boost::asio::io_context&,
                                        const std::string host, int port);

class Client : public std::enable_shared_from_this<Client> {
 public:
  Client(boost::asio::io_context& io_context,
         boost::asio::ip::tcp::socket socket);

  void SetHandlerReceivingMessage(handlers::HandlerReceivingMessage handler);
  // void SendMessage(const std::string&);
  // void SendMessage(char* data, uint32_t length);

  void PushTask(std::unique_ptr<Task>);

  void Start();
  void Stop();

 private:
  void DoTask();

  void AsyncReadHeader();
  void AsyncReadBody();
  void AsyncSendHeader();
  void AsyncSendBody();

  void OnReadHeader(const boost::system::error_code& error,
                    size_t bytes_transferred);
  void OnReadBody(const boost::system::error_code& error,
                  size_t bytes_transferred);
  void OnSendHeader(const boost::system::error_code& error,
                    size_t bytes_transferred);
  void OnSendHeader(const boost::system::error_code& error,
                    size_t bytes_transferred);

 private:
  boost::asio::io_context& io_context_;
  boost::asio::ip::tcp::socket socket_;

  // std::deque<Message> writing_queue_;
  std::deque<std::unique_ptr<Task>> task_queue;
  Message reading_;

  handlers::HandlerReceivingMessage handler_receiving_message_;
};

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
