#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cerrno>
#include <clocale>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "eventhandler.h"
#include "timeutils.h"

// CONTROL MESSAGE MACROS

#define SESSION_START 0
#define SESSION_END 1
#define SESSION_TAG 2
#define HANDSHAKE_OK 3

/**
 * The socketServer class handles communication for the server side (meter side)
 */
class socketServer {
 public:
  socketServer(uint16_t portNumber, eventHandler* handler);

  ~socketServer();

  // This blocks until it receives a connection from a client then continues to
  // receive data from the client.
  void listenForClient();

 private:
  // This is the file descriptor of the socket.
  int sock;
  eventHandler* handler;

  // This stores information about the server that is being connected to.
  sockaddr_in address;

  // This is a wrapper for socket read that stores the data in the given buffer.
  void readData(int socketFD, void* buf, size_t size);

  // This is a wrapper for socket write that sends data from the given buffer.
  void writeData(int socketFD, void* buf, size_t size);

  // This function interprets the data received in listenForClient() and calls
  // the correct function to handle the communication.`
  int handleClientConnection(int readSocket);

  // This does the things needed at a session start such as starting
  // the meter and taking the first timestamp.
  void handleSessionStart(int readSocket);

  // This does the things needed at the end of a session such as
  // stopping the meter and dumping the timestamps and meter readings to a file.
  void handleSessionEnd(int readSocket);

  // This marks the timestamp and string of a tag that has been
  // received.
  void handleTag(int socketFD);
};

/**
 * The socketServer class handles communication for the client side (benchmark
 * side)
 */
class socketClient {
 public:
  socketClient(uint16_t portNumber, std::string serverIP);
  socketClient();

  ~socketClient();

  // This lets the server know to start measuring.
  void sendSessionStart();

  // This lets the server know to stop measuring.
  void sendSessionEnd();

  // This tells the server to tag a specific time in the measurements while
  // something of note is happening.
  void sendTag(std::string tagName);

 private:
  // This is the file descriptor of the socket.
  int sock;

  // This stores information about the server that is being connected to.
  sockaddr_in serverAddress;

  // This is a wrapper for socket read that stores the data in the given buffer.
  void readData(void* buf, size_t size);

  // This is a wrapper for socket write that sends data from the given buffer.
  void writeData(void* buf, size_t size);
};

// This function prints an error message, prints the last errorno, and exits the
// program.
void printError(std::string errorMsg);

#endif