//! Worker interface.
//!
//! \file worker.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <chrono>
#include <cstdint>
#include <future>
#include <vector>

#include <cppzmq/zmq.hpp>

#define STATELINE_PROTOCOL_VERSION 0

namespace stateline
{

using JobID = unsigned int;
using JobType = unsigned int;

namespace detail
{

constexpr int NUM_IO_THREADS = 2;

template <class T>
void writePrimitive(std::string& s, T val)
{
  std::string buf{reinterpret_cast<char *>(&val), sizeof(val)};
  s.append(std::move(buf));
}

template <class T>
T readPrimitive(const char* buf)
{
  return *reinterpret_cast<T *>(buf);
}


//! Thin wrapper around a TCP ZMQ_STREAM socket. Provides functions to connect
//! to the server and send messages.
//!
class TcpSocket
{
private:
  //! The maximum size of a ZMQ identity
  static constexpr std::size_t maxIdSize() { return 256; }

public:
  //! Constructs a new TCP socket.
  //!
  //! \param numIOThreads Number of threads to use for IO. Should be at least one.
  //!
  TcpSocket(int numIOThreads)
    : ctx_{numIOThreads}
    , socket_{ctx_, ZMQ_STREAM}
    , serverIdentity_(maxIdSize()) // Allocate buffer for server identity
  {
  }

  TcpSocket(const TcpSocket&) = delete;

  //! Connect to a host.
  //!
  //! \param address The address of the host, without the protocol prefix (e.g. localhost:5000)
  //!
  void connect(const std::string& address)
  {
    socket_.connect("tcp://" + address);

    // When sending through ZMQ_STREAM sockets, we need to send the identity of the
    // server as the first message part.
    std::size_t idSize;
    socket_.getsockopt(ZMQ_IDENTITY, serverIdentity_.data(), &idSize);
    serverIdentity_.resize(idSize);
  }

  //! Send a string. connect() must be called prior to calling this method.
  //!
  //! \params data The bytes to send.
  //!
  void send(const std::string& data)
  {
    // Send the header first (zero-copy)
    zmq::message_t header{serverIdentity_.data(), serverIdentity_.size(), NULL};
    socket_.send(header, ZMQ_SNDMORE);

    // Send the payload message
    zmq::message_t msg{data.size()};
    memcpy(msg.data(), data.data(), data.size()); // TODO: eliminate this copy
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
  std::vector<std::uint8_t> serverIdentity_;
};

//! Provides a layer above a socket that can understand the Stateline protocol.
//!
template <class Socket>
class MessageHandler
{
private:
  struct JobHeader
  {
    std::uint8_t msgType = 0x2;
    std::uint32_t id;
    std::uint32_t type;
  };

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

  void sendHeartbeat()
  {
    socket_.send("\x00");
  }

  void sendHello()
  {
    std::string buf;
    writePrimitive(buf, std::uint8_t{1});                          // Message type
    writePrimitive(buf, std::uint8_t{STATELINE_PROTOCOL_VERSION}); // Version
    writePrimitive(buf, std::uint32_t{0});                         // Job type from
    writePrimitive(buf, std::uint32_t{0});                         // Job type to

    socket_.send(buf);
  }

  Job recvJob()
  {
    const auto buf = socket_.recv();
    readPrimitive<std::uint8_t>(buf.data());                  // Message type
    auto id = readPrimitive<std::uint32_t>(buf.data() + 1);   // Job ID
    auto type = readPrimitive<std::uint32_t>(buf.data() + 5); // Job type

    // The remaining bytes in the buffer is the job data
    std::vector<double> data(buf.size() - 9);
    memcpy(data.data(), buf.data() + sizeof(JobHeader), data.size()); // TODO: remove this copy

    return {id, type, std::move(data)};
  }

  void sendResult(std::uint32_t id, double data)
  {
    std::string buf;
    writePrimitive(buf, std::uint8_t{3});   // Message type
    writePrimitive(buf, std::uint32_t{id}); // Version
    writePrimitive(buf, double{data});      // Data

    socket_.send(std::move(buf));
  }

private:
  Socket& socket_;
};

template <class Socket>
void heartbeat(MessageHandler<Socket>& handler, int ms)
{
  while (false) // TODO: interrupt flag
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    handler.sendHeartbeat();
  }
}

}

template <class Nll>
void runWorker(const std::string& address, Nll nll)
{
  detail::TcpSocket socket{detail::NUM_IO_THREADS};
  socket.connect(address);

  detail::MessageHandler<detail::TcpSocket> handler{socket};

  // Send hello message to initiate the protocol
  handler.sendHello();

  // TODO: Start heartbeats

  /*
  while (false) // TODO: interrupt flag
  {
    const auto job = handler.recvJob();
    const auto result = nll(job.type, job.data);

    handler.sendResult(job.id, result);
  }*/
}

}
