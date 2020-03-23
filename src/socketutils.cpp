#include "socketutils.h"

/**
 * Print an error message to stderr and exit with failure code
 *
 * @param errorMsg the message to write to standard error
 */
void printError(std::string errorMsg) {
  std::cerr << errorMsg << std::strerror(errno) << "\n";
  exit(EXIT_FAILURE);
}

/**
 * Constructor for socket server. Set up sockets for communication.
 *
 * @param portNumber the port number of the socket that the server will listen
 * on
 * @param eventHandler the event handler that will respond to communication
 * events
 */
socketServer::socketServer(uint16_t portNumber, eventHandler *eventHandler) {
  handler = eventHandler;

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

/**
 * Destructor for socket server. Closes its socket
 */
socketServer::~socketServer() { close(sock); }

/**
 * Reads data from a given socket into a buffer
 * @param socketFD the socket file descriptor that is read from
 * @param buf the buffer that the data will be written to
 * @param size the size of the data to be read and moved to buf
 */
void socketServer::readData(int socketFD, void *buf, size_t size) {
  int *tmp = (int *)buf;
  size_t to_read = size;
  ssize_t numRead = 0;
  do {
    if ((numRead = read(socketFD, tmp, to_read)) == -1) {
      printError(
          "Server failed to completely read from the socket with errorno: ");
    }
    to_read -= numRead;
    tmp += numRead;
  } while (to_read);
}

/**
 * Reads data from a given socket into a buffer
 * @param socketFD the socket file descriptor that is written to
 * @param buf the buffer that will be written to the socket
 * @param size the size of the data to be written to the socket
 */
void socketServer::writeData(int socketFD, void *buf, size_t size) {
  ssize_t bytesSent;
  if ((bytesSent = send(socketFD, buf, size, 0)) < size) {
    printError("Server failed to completely send on socket. Server sent " +
               std::to_string(bytesSent) + " bytes, but was expected to send " +
               std::to_string(size) + " bytes.");
  }
}

/**
 * Establish a server client connection and handle client requests until
 * completion.
 */
void socketServer::listenForClient() {
  socklen_t clientLength;
  int readSocket;

  if (listen(sock, 2) == -1) {
    printError("Server failed to listen on socket");
  }

  clientLength = sizeof(address);
  if ((readSocket = accept(sock, (sockaddr *)&address, &clientLength)) == -1) {
    printError("Error, the accept failed with errno: ");
  }
  // Once the connection has been accepted, keep reading until
  // handleClientConnection() gives an error.
  while (handleClientConnection(readSocket) == 0)
    ;
}

/**
 * Handles an arbitrary communication from a client. Responsible for calling the
 * correct handler for each communication.
 * @param readSocket the socket that communications can be read from
 */
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

/**
 * Process the "start session" communication. Pass the start session time stamp
 * to the start session handler.
 * @param readSocket the socket that communications can be read from
 */
void socketServer::handleSessionStart(int readSocket) {
  uint64_t timestamp;
  read(readSocket, &timestamp, sizeof(uint64_t));

  socketServer::handler->startHandler(timestamp);

  char response = HANDSHAKE_OK;
  writeData(readSocket, &response, sizeof(char));
}

/**
 * Process the "end session" communication. Pass the end session time stamp to
 * the end session handler.
 * @param readSocket the socket that communications can be read from
 */
void socketServer::handleSessionEnd(int readSocket) {
  uint64_t timestamp;
  read(readSocket, &timestamp, sizeof(uint64_t));

  socketServer::handler->endHandler(timestamp);
}

/**
 * Process a single tag. Pass the tag's timestamp and message to the tag
 * handler.
 * @param socketFD the socket that the tag is sent to
 */
void socketServer::handleTag(int socketFD) {
  // Timestamps is pushed with an empty string so that the nanoseconds timestamp
  // can be recorded as accurately as possible.

  // This receives the size of the string that will be transmitted.
  size_t tagSize;
  readData(socketFD, &tagSize, sizeof(size_t));

  // This receives the tag string from the client.
  char *message = (char *)calloc(tagSize, sizeof(char));
  readData(socketFD, message, tagSize);

  // Receive timestamp
  uint64_t timestamp;
  readData(socketFD, &timestamp, sizeof(uint64_t));

  socketServer::handler->tagHandler(timestamp, message);

  free(message);
}

//####################################################################

/**
 * Constructor for socket client. Establishes connection with server
 * @param portNumber the port number used for communication
 * @param serverIP the ip address used for communication
 */
socketClient::socketClient(uint16_t portNumber, std::string serverIP) {
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

/**
 * Destructor for socket client that closes its socket
 */
socketClient::~socketClient() { close(sock); }

/**
 * Helper that allows the client to read data from its socket
 * @param buf the buffer that data will be read into
 * @param size the size of the data to be read into buf
 */
void socketClient::readData(void *buf, size_t size) {
  if (read(sock, buf, size) == -1) {
    printError("Client failed to completely read from socket\n");
  }
}

/**
 * Helper that allows the client to write arbitrary data to its socket
 * @param buf the buffer containig the data to be sent
 * @param size the size of the data to be sent from buf
 */
void socketClient::writeData(void *buf, size_t size) {
  int bytesSent;

  if ((bytesSent = send(sock, buf, size, 0)) <= 0) {
    printError("Client failed to completely send on socket. Client sent " +
               std::to_string(bytesSent) + " bytes, but expected to send " +
               std::to_string(size) + " bytes: ");
  }
}

/**
 * Sesnds the "start session" communication from client to server
 */
void socketClient::sendSessionStart() {
  char buffer[512];
  uint64_t currTime = nanos();
  char tagBuf = SESSION_START;
  size_t position = 0;

  memcpy(buffer + position, &tagBuf, sizeof(char));
  position += sizeof(char);

  memcpy(buffer + position, &currTime, sizeof(uint64_t));
  position += sizeof(uint64_t);

  writeData(buffer, position);

  char response;
  readData(&response, sizeof(char));

  if (response != HANDSHAKE_OK) {
    std::cerr << "Handshake with server failed!" << std::endl;
    exit(-1);
  }
  std::cout << "Handshake OK!" << std::endl;
}

/**
 * Sends the "end session" communication from client to server
 */
void socketClient::sendSessionEnd() {
  char buffer[512];
  uint64_t currTime = nanos();
  char tagBuf = SESSION_END;
  size_t position = 0;

  memcpy(buffer + position, &tagBuf, sizeof(char));
  position += sizeof(char);

  memcpy(buffer + position, &currTime, sizeof(uint64_t));
  position += sizeof(uint64_t);

  writeData(buffer, position);
}

/**
 * Sends a tag of tagName from client to sever
 *
 * @param tagName the label given to the sent tag
 */
void socketClient::sendTag(std::string tagName) {
  char buffer[512];
  uint64_t currTime = nanos();
  char tagBuf = SESSION_TAG;
  size_t position = 0;
  size_t tagSize = tagName.size() + 1;

  memcpy(buffer + position, &tagBuf, sizeof(char));
  position += sizeof(char);

  memcpy(buffer + position, &tagSize, sizeof(size_t));
  position += sizeof(size_t);

  memcpy(buffer + position, tagName.c_str(), tagName.size() + 1);
  position += tagName.size() + 1;

  memcpy(buffer + position, &currTime, sizeof(uint64_t));
  position += sizeof(uint64_t);

  writeData(buffer, position);
}
