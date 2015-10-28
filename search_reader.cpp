/// program gets Nodes data from the solver and populates database with that

#include <string>
#include <iostream>

#include <fstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include "third-party/zmq.hpp"

#include "message.pb.hh"

using namespace std;
using google::protobuf::io::IstreamInputStream;

bool readDelimitedFrom(
    google::protobuf::io::ZeroCopyInputStream* rawInput,
    google::protobuf::MessageLite* message) {
  // We create a new coded stream for each message.  Don't worry, this is fast,
  // and it makes sure the 64MB total size limit is imposed per-message rather
  // than on the whole stream.  (See the CodedInputStream interface for more
  // info on this limit.)
  google::protobuf::io::CodedInputStream input(rawInput);

  // Read the size.
  uint32_t size;
  if (!input.ReadVarint32(&size)) return false;

  // Tell the stream not to read beyond that size.
  google::protobuf::io::CodedInputStream::Limit limit =
      input.PushLimit(size);

  // Parse the message.
  if (!message->MergeFromCodedStream(&input)) return false;
  if (!input.ConsumedEntireMessage()) return false;

  // Release the limit.
  input.PopLimit(limit);

  return true;
}

void sendOverSocket(zmq::socket_t &socket, const message::Node &msg) {
  std::string msg_str;
  msg.SerializeToString(&msg_str);

  zmq::message_t request(msg_str.size());
  memcpy((void*)request.data(), msg_str.c_str(), msg_str.size());

  // Sometimes sending will fail with EINTR.  In this case, we try to
  // send the message again.
  while (true) {
    int failed_attempts = 0;
    try {
      bool sentOK = socket.send(request);
      // If sentOK is false, there was an EAGAIN.  We handle this the
      // same as EINTR.
      if (!sentOK) {
        failed_attempts++;
        if (failed_attempts > 10) abort();
        continue;
      }
      // Success: stop the loop.
      break;
    } catch (zmq::error_t &e) {
      failed_attempts++;
      if (failed_attempts > 10) abort();
      if (e.num() == EINTR) {
        continue;
      }
      // If it was something other than EINTR, rethrow the exception.
      throw e;
    }
  }
}

std::string statusToString(message::Node::NodeStatus status) {
  return "";
}


int main(int argc, char** argv) {

  bool DEBUG = false;

  string path;

  if (argc == 2) {
    path = argv[1];
    std::cout << "File location: " << path << std::endl;
  } else if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
    path = argv[1];
    DEBUG = true;
  } else {
    std::cerr << "Usage: search_reader [FILE]" << std::endl;
    return 1;
  }


  std::ifstream inputFile(path);
  IstreamInputStream raw_input(&inputFile);

  if (!inputFile) {
    std::cerr << "Can't read the file" << std::endl;
    return 1;
  }

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_PUSH);

  std::string address = "tcp://localhost:6565";
  socket.connect(address.c_str());

  message::Node msg;

  while (true) {

    readDelimitedFrom(&raw_input, &msg);

    if (DEBUG) {
      cout << "Enter to send a node..." << endl;
      std::string temp;
      getline(cin, temp);
      switch (msg.type()) {
        case message::Node::START:
          std::cout << "Start receiving...\n";
          break;
        case message::Node::NODE:
          std::cout << "sid: " << msg.sid() << "\tpid: " << msg.pid() << "\talt: " << msg.alt()
            << "\tkids: " << msg.kids() << "\tstatus: " << statusToString(msg.status())
            << "\tlabel: " << msg.label() << "\tthread: " << msg.thread_id()
            << "\trestart: " << msg.restart_id() << std::endl;
        case message::Node::DONE:
          std::cout << "Done receiving.\n";
        break;
      }
    }

    sendOverSocket(socket, msg);

    if (msg.type() == message::Node::DONE)
      return 0;

  }

  return 0;
}