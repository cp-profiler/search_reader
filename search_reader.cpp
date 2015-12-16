/// program gets Nodes data from the solver and populates database with that

#include <string>
#include <iostream>

#include <fstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "message.pb.hh"
#include "cpp-integration/connector.hh"

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

std::string statusToString(message::Node::NodeStatus) {
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

  Profiling::Connector c(6565);
  c.connect();

  message::Node msg;

  while (true) {

    bool ok = readDelimitedFrom(&raw_input, &msg);
    if (!ok) break;

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

    std::string s;
    msg.SerializeToString(&s);
    c.sendRawMsg(s.c_str(), s.size());

    if (msg.type() == message::Node::DONE)
        break;
  }

  return 0;
}
