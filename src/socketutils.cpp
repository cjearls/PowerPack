#include "socketutils.h"

void printError(std::string errorMsg) {
  std::cerr << errorMsg << std::strerror(errno) << "\n";
  exit(EXIT_FAILURE);
}

socketServer::socketServer(int portNumber, eventHandler *handler) {
  socketServer::handler = handler;
  std::cerr << portNumber;

  // This causes the connection to be IPv4.
  address.sin_family = AF_INET;
  // This allows any address connection.
  address.sin_addr.s_addr = INADDR_ANY;
  // This resolves the given port number.
  address.sin_port = htons(portNumber);
  int opt = 1;

  // This sets the socket to use TCP.
  if ((sock = socket(address.sin_family, SOCK_STREAM, 0)) == 0) {
    printError("Server failed to initialize socket\n");
  }

  // This sets some options for the socket, telling it to reuse the address and
  // port it was given.
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    printError("Server failed to set socket options\n");
  }
  
  // This binds the socket to the given address details.
  if (bind(sock, (sockaddr *)&address, sizeof(address)) < 0) {
    printError("Server failed to bind to socket\n");
  }
}

socketServer::~socketServer() { close(sock); }

void socketServer::readData(int socketFD, void *buf, size_t size) {
  int* tmp = (int*)buf;
  size_t to_read = size;
  size_t numRead = 0;
  do{
    if ( (numRead = read(socketFD, tmp, to_read) ) == -1) {
      printError(
          "Server failed to completely read from the socket with errorno: ");
    }
    to_read -= numRead;
    tmp += numRead;
  }while(to_read);
}

void socketServer::write(int socketFD, void *buf, size_t size) {
  size_t bytesSent;
  if ((bytesSent = send(socketFD, buf, size, 0)) < size) {
    printError("Server failed to completely send on socket. Server sent " +
               std::to_string(bytesSent) + " bytes, but was expected to send " +
               std::to_string(size) + " bytes.");
  }
}

void socketServer::listenForClient() {
  socklen_t clientLength;
  int readSocket;
  int iMode = 1;

  if (listen(sock, 2) == -1) {
    printError("Server failed to listen on socket");
  }

  clientLength = sizeof(address);
  if ((readSocket = accept(sock, (sockaddr *)&address, (socklen_t*)&clientLength)) == -1) {
    printError("Error, the accept failed with errno: ");
  }
  // Once the connection has been accepted, keep reading until
  // handleClientConnection() gives an error.
  while (handleClientConnection(readSocket) == 0)
    ;
}

int socketServer::handleClientConnection(int readSocket) {
  char msgTypeBuffer;
  readData(readSocket, &msgTypeBuffer, sizeof(char));
  switch (msgTypeBuffer) {
    case SESSION_START:
      handleSessionStart(readSocket);
      break;

    case SESSION_END:
      handleSessionEnd(readSocket);
      return 1;

    case SESSION_TAG:
      handleTag(readSocket);
      break;

    default:
      std::cerr << msgTypeBuffer << std::endl;
      printError("received unknown msg code");
  }

  return 0;
}

void socketServer::handleSessionStart(int readSocket) {
  
  // TODO: start the meter
  socketServer::handler->startHandler();

  uint64_t timestamp;
  read(readSocket, &timestamp, sizeof(uint64_t));
  timestamps.emplace_back("Starting Session...", timestamp);
  char response = HANDSHAKE_OK;
  write(readSocket, &response, sizeof(char));
}

void socketServer::handleSessionEnd(int readSocket) {
  uint64_t timestamp;
  read(readSocket, &timestamp, sizeof(uint64_t));
  timestamps.emplace_back("Ending Session...", timestamp);
  // TODO: stop the meter

socketServer::handler->endHandler();

  // TODO: dump to a file instead of to cout
  for (size_t index = 0; index < timestamps.size(); index++) {
    std::cout << "Tag: \"" << timestamps[index].first << "\": " 
              << timestamps[index].second - timestamps[0].second
              << " nanoseconds after progam start.\n";
  }  
}

void socketServer::handleTag(int socketFD) {
  // Timestamps is pushed with an empty string so that the nanoseconds timestamp
  // can be recorded as accurately as possible.

  // This receives the size of the string that will be transmitted.
  size_t tagSize;
  readData(socketFD, &tagSize, sizeof(size_t));

  // This receives the tag string from the client.
  char* msgBuffer = (char*)calloc(tagSize, sizeof(char));
  readData(socketFD, msgBuffer, tagSize);

  uint64_t currTime;
  readData(socketFD, &currTime, sizeof(uint64_t));

  // The correct tag is given to the timestamp.
  timestamps.emplace_back(msgBuffer, currTime);

  free(msgBuffer);
  socketServer::handler->tagHandler();
}

socketClient::socketClient(int portNumber, std::string serverIP) {
  // This sets the socket to IPv4 and to the port number given.
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(portNumber);

  if ((sock = socket(serverAddress.sin_family, SOCK_STREAM, 0)) < 0) {
    printError("Client socket creation error\n");
  }

  // This checks the address to be sure it is allowed.
  if (inet_pton(serverAddress.sin_family, serverIP.c_str(),
                &serverAddress.sin_addr) == -1) {
    printError("Invalid address or address not supported\n");
  }

  // This connects to the server.
  if (connect(sock, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    printError("Client connection failed: ");
  }
}

socketClient::~socketClient() { close(sock); }

void socketClient::readData(void *buf, size_t size) {
  if (read(sock, buf, size) == -1) {
    printError("Client failed to completely read from socket\n");
  }
}

void socketClient::write( void *buf, size_t size) {
  int bytesSent;

  if ((bytesSent = send(sock, buf, size, 0))  <= 0 ) {
    printError("Client failed to completely send on socket. Client sent " +
               std::to_string(bytesSent) + " bytes, but expected to send " +
               std::to_string(size) + " bytes: ");
  }
}

void socketClient::sendSessionStart() {
  char __buffer [512];
  void* buffer = (void*) __buffer;
  uint64_t currTime = nanos();
  char tagBuf = SESSION_START;
  size_t position = 0;

  memcpy(buffer+position, &tagBuf, sizeof(char));
  position += sizeof(char);

  memcpy(buffer+position, &currTime, sizeof(uint64_t));
  position += sizeof(uint64_t);

  write(buffer, position);

  char response;
  readData(&response, sizeof(char));

  if(response != HANDSHAKE_OK) {
    std::cerr << "Handshake with server failed!" << std::endl;
    exit(-1);
  }
  std::cout << "Handshake OK!" << std::endl;
}

void socketClient::sendSessionEnd() {
  char __buffer [512];
  void* buffer = (void*) __buffer;
  uint64_t currTime = nanos();
  char tagBuf = SESSION_END;
  size_t position = 0;

  memcpy(buffer+position, &tagBuf, sizeof(char));
  position += sizeof(char);

  memcpy(buffer+position, &currTime, sizeof(uint64_t));
  position += sizeof(uint64_t);

  write(buffer, position);
}

void socketClient::sendTag(std::string tagName) {
  char __buffer [512];
  void* buffer = (void*) __buffer;
  uint64_t currTime = nanos();
  char tagBuf = SESSION_TAG;
  size_t position = 0;
  size_t tagSize = tagName.size() + 1;

  memcpy(buffer+position, &tagBuf, sizeof(char));
  position += sizeof(char);

  memcpy(buffer+position, &tagSize, sizeof(size_t));
  position += sizeof(size_t);

  memcpy(buffer+position, tagName.c_str(), tagName.size() + 1);
  position += tagName.size() + 1;

  memcpy(buffer+position, &currTime, sizeof(uint64_t));
  position += sizeof(uint64_t);

  write(buffer, position);
}
