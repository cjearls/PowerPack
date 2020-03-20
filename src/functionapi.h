#ifndef FUNCTION_API_H
#define FUNCTION_API_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "socketutils.h"
/*
 * API For users who want to generate a power profile for their functions
 * Client and server are seperate processes that communicate over a tcp socket
 * Client sends timestamped tags to server
 * Server handles data collection via ni-meter and
 */

socketServer initializeMeterServer(uint16_t portNumber, eventHandler* handler);
void closeMeterServer();

socketClient initializeFunctionClient(uint16_t port, std::string serverAddress);
void closeFunctionClient();

// Create a list of key_value pair from an input config file
std::unordered_map<std::string, std::string> createConfigurationMap(
    std::string configPath);

// Turn a string into a key-value pair at the delimiter
std::pair<std::string, std::string> findPair(std::string inputString,
                                             char delimiter);

double* stringToDoubleArray(std::string str);

// Wrapper class for an unordered map of configuration values
class Configuration {
 public:
  std::unordered_map<std::string, std::string> map;
  Configuration(std::string configFile);
  ~Configuration();

  // Get a value corresponding to a given key
  std::string get(std::string key);

  // Print all key-value pairs
  std::string toString();
};

#endif