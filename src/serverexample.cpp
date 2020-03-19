#include <thread>
#include "functionapi.h"
#include "nidaqmxeventhandler.h"

int main(int argc, char** argv) {
  std::string configFile(argv[1]);
  NIDAQmxEventHandler niHandler(argv[2]);
  eventHandler* handler;
  handler = &niHandler;

  Configuration configuration = Configuration(configFile);

  int port = stoi(configuration.get("port"), nullptr, 10);

  std::cout << configuration.toString();

  socketServer server =
     initializeMeterServer(port , handler);
  server.listenForClient();

  return 0;
}
