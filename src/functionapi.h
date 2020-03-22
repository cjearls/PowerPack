#ifndef FUNCTION_API_H
#define FUNCTION_API_H

#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include "socketutils.h"

/*
 * API For users who want to generate a power profile for their functions
 * Client and server are seperate processes that communicate over a tcp socket
 * Client sends timestamped tags to server
 * Server handles data collection via ni-meter and
 *
 * TODO: DO WE NEED THIS EXTRA LAYER THOUGH
 */

// initialize the server side of powerpack
socketServer initializeMeterServer(int portNumber, eventHandler *handler);
// close meter side of powepack
void closeMeterServer();
// read config file for server parameters
int readServerConfig(std::string configFilePath);

// initialize the client side of powerpack
socketClient initializeFunctionClient(
    std::pair<int, std::string> clientNetworkInfo);

// close client side of powerpack
void closeFunctionClient();

// send tag communicatino from client to server
void issueTag(std::string tagName);

// read config file for client parameters
std::pair<int, std::string> readClientConfig(std::string configFilePath);

// helper that determines if substring is a substring of inputstring
bool isSubString(std::string inputString, std::string subString);

// helper that pulls config values out of input string
std::string extractConfigValue(std::string inputString);

#endif