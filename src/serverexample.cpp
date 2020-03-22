#include <thread>
#include "functionapi.h"
#include "nidaqmxeventhandler.h"


/**
 * Read server config info from configFile and initialize a server. Used as basic test of powerpack functionality
 */ 
int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <config file> <output file>"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string configFile(argv[1]);
  NIDAQmxEventHandler niHandler(argv[2]);
  eventHandler* handler;
  handler = &niHandler;
  Configuration configuration = Configuration(configFile);

  niHandler.configure(configuration);

  uint16_t port = stoi(configuration.get("port"), nullptr, 10);

  std::cout << configuration.toString();

  socketServer server = initializeMeterServer(port, handler);
  server.listenForClient();

  return 0;
}
