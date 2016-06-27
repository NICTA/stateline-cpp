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
  void send(std::string data)
  {
    // Send the header first (zero-copy)
    zmq::message_t header{serverIdentity_.data(), serverIdentity_.size(), NULL};
    socket_.send(header, ZMQ_SNDMORE);

    // Send the payload message
    zmq::message_t msg{&data[0], data.size(), NULL};
    socket_.send(msg, 0);
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
  struct Hello
  {
    std::uint8_t msgType = 0x1;
    std::uint8_t version;
    std::uint32_t jobTypeFrom;
    std::uint32_t jobTypeTo;
  };

  struct JobHeader
  {
    std::uint8_t msgType = 0x2;
    std::uint32_t id;
    std::uint32_t type;
  };

  struct Result
  {
    std::uint8_t msgType = 0x3;
    std::uint32_t id;
    double data;
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
    static_assert(std::is_trivially_copyable<Hello>::value, "'Hello' must be trivially copyable");

    Hello msg;
    msg.version = STATELINE_PROTOCOL_VERSION;

    std::string buf{reinterpret_cast<char *>(&msg), sizeof(msg)}; // TODO: eliminate copy with string_ref
    socket_.send(std::move(buf));
  }

  Job recvJob()
  {
    static_assert(std::is_trivially_copyable<JobHeader>::value, "'JobHeader' must be trivially copyable");

    const auto buf = socket_.recv();

    assert(buf.size() >= sizeof(JobHeader));
    const auto job = reinterpret_cast<const JobHeader *>(buf.data());

    // The remaining bytes in the buffer is the job data
    std::vector<double> data(buf.size() - sizeof(JobHeader));
    memcpy(data.data(), buf.data() + sizeof(JobHeader), data.size()); // TODO: remove this copy

    return {job->id, job->type, std::move(data)};
  }

  void sendResult(std::uint32_t id, double data)
  {
    static_assert(std::is_trivially_copyable<Result>::value, "'Result' must be trivially copyable");

    Result msg;
    msg.id = id;
    msg.data = data;

    std::string buf{reinterpret_cast<char *>(&msg), sizeof(msg)};
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

  while (false) // TODO: interrupt flag
  {
    const auto job = handler.recvJob();
    const auto result = nll(job.type, job.data);

    handler.sendResult(job.id, result);
  }
}

}
