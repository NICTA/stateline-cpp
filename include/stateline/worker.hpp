//! Worker interface.
//!
//! \file worker.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <future>
#include <type_traits>
#include <vector>

#include <cppzmq/zmq.hpp>

namespace stateline
{

using JobID = unsigned int;
using JobType = unsigned int;

namespace detail
{

constexpr int NUM_IO_THREADS = 2;

template <class... Args>
struct PackSize;

template <class T, class... Args>
struct PackSize<T, Args...>
{
  static constexpr std::size_t value = sizeof(T) + PackSize<Args...>::value;
};

template <>
struct PackSize<> { static constexpr std::size_t value = 0; };

char* packBuffer(char* buf)
{
  return buf;
}

template <class T, class... Args>
char* packBuffer(char* buf, T val, Args... args)
{
  memcpy(buf, reinterpret_cast<char*>(&val), sizeof(val));
  return packBuffer(buf + sizeof(val), args...);
}

template <class... Args>
std::array<char, PackSize<Args...>::value> packArray(Args... args)
{
  std::array<char, PackSize<Args...>::value> buffer;
  packBuffer(buffer.data(), args...);
  return buffer;
}

// TODO: there's gotta be a better way of doing this
template <class... Args>
typename std::enable_if<
  sizeof...(Args) == 0,
  std::pair<std::tuple<>, const char*>
>::type unpackBuffer(const char* buf)
{
  return {{}, buf};
}

template <class T, class... Args>
std::pair<std::tuple<T, Args...>, const char*> unpackBuffer(const char* buf)
{
  T val = *reinterpret_cast<const T*>(buf);

  auto result = unpackBuffer<Args...>(buf + sizeof(T));
  return {
      std::tuple_cat(std::tuple<T>(val), result.first),
      result.second
  };
}

//! Thin wrapper around a IPC socket to connect to the server and send messages.
//!
class IpcSocket
{
public:
  //! Constructs a new IPC socket.
  //!
  //! \param numIOThreads Number of threads to use for IO. Should be at least one.
  //!
  IpcSocket(int numIOThreads)
    : ctx_{numIOThreads}
    , socket_{ctx_, ZMQ_REQ}
  {
  }

  IpcSocket(const IpcSocket&) = delete;

  //! Connect to a host.
  //!
  //! \param address The address of the host.
  //!
  void connect(const std::string& address)
  {
    socket_.connect(address);
  }

  //! Send a buffer. connect() must be called prior to calling this method.
  //!
  //! \params data The bytes to send.
  //!
  void send(const char* buf, std::size_t size)
  {
    // Send the payload message
    zmq::message_t msg{size};
    memcpy(msg.data(), buf, size);
    socket_.send(msg);
  }

  std::string recv()
  {
    zmq::message_t msg;
    socket_.recv(&msg);
    return {static_cast<char *>(msg.data()), msg.size()}; // TODO: can we eliminate the copy here?
  }

private:
  zmq::context_t ctx_;
  zmq::socket_t socket_;
};

//! Provides a layer above a socket that can understand the Stateline protocol.
//!
template <class Socket>
class MessageHandler
{
public:
  struct Job
  {
    JobID id;
    JobType type;
    std::vector<double> data;
  };

  MessageHandler(Socket& socket)
    : socket_(socket)
  {
  }

  void sendHello(JobType from, JobType to)
  {
    auto buf = packArray(
      std::uint8_t{1},                            // Message type
      std::uint32_t{from},                        // Job type from
      std::uint32_t{to}                           // Job type to
    );

    socket_.send(buf.data(), buf.size());
  }

  Job recvJob()
  {
    // TODO: can we assert the size of the buffer here?
    const auto buf = socket_.recv();

    auto result = unpackBuffer<
      std::uint8_t,   // Message type
      std::uint32_t,  // Job ID
      std::uint32_t   // Job type
    >(buf.data());

    // The remaining bytes in the buffer is the job data
    assert(result.second - buf.data() < buf.size());
    std::vector<double> data(buf.size() - (result.second - buf.data()));
    memcpy(data.data(), result.second, data.size());

    return {
      std::get<1>(result.first),
      std::get<2>(result.first),
      std::move(data)
    };
  }

  void sendResult(std::uint32_t id, double data)
  {
    auto buf = packArray(
      std::uint8_t{5},   // Message type
      std::uint32_t{id}, // Job ID
      double{data}       // Data
    );

    socket_.send(buf.data(), buf.size());
  }

private:
  Socket& socket_;
};

}

template <class Nll>
void runWorker(const std::string& address, Nll nll)
{
  detail::IpcSocket socket{detail::NUM_IO_THREADS};
  socket.connect(address);
  std::cout << "Connected to " << address << std::endl;

  detail::MessageHandler<detail::IpcSocket> handler{socket};

  // Send hello message to initiate the protocol
  handler.sendHello(0, 0);

  for (int i = 0; i < 1; i++) // TODO: interrupt flag
  {
    const auto job = handler.recvJob();
    const auto result = nll(job.type, job.data);

    handler.sendResult(job.id, result);
  }
}

}
