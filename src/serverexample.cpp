#include <thread>
#include "functionapi.h"
#include "nidaqmxeventhandler.h"


/**
 * Read server config info from configFile and initialize a server. Used as basic test of powerpack functionality
 */ 
int main(int argc, char** argv) {
  std::string configFile(argv[1]);
  NIDAQmxEventHandler niHandler(argv[2]);
  eventHandler* handler;
  handler = &niHandler;

  socketServer server =
      initializeMeterServer(readServerConfig(configFile), handler);
  server.listenForClient();

  return 0;
}
