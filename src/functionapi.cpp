#include "functionapi.h"

socketServer initializeMeterServer(int portNumber, eventHandler* handler) {
  socketServer server(portNumber, handler);
  return server;
}

socketClient initializeFunctionClient(int port, std::string serverAddress) {
  socketClient client(port, serverAddress);
  return client;
}

Configuration::Configuration(std::string configFile) {
  map = createConfigurationMap(configFile);
}

Configuration::~Configuration(){};

std::string Configuration::get(std::string key) {
  if (map.find(key) == map.end()) {
    std::cerr << key << " not specified in configuration." << std::endl;
    exit(EXIT_FAILURE);
  }

  return map.at(key);
}

std::string Configuration::toString() {
  std::stringstream ret;  // return value

  for (auto& entry : map) {
    ret << entry.first << ": " << entry.second << "\n";
  }
  return ret.str();
}

std::unordered_map<std::string, std::string> createConfigurationMap(
    std::string configPath) {
  // Input stream for reading file
  std::ifstream configFileStream;
  // Temporary variable to hold incoming lines
  std::string tempString;
  // Current key-vale pair being parsed
  std::pair<std::string, std::string> currPair;
  // Map of key-value pair from the input file
  std::unordered_map<std::string, std::string> configuration;

  configFileStream.open(configPath);

  if (!configFileStream.good()) {
    std::cerr << "Error opening" << configPath
              << ". Please make sure the file exists." << std::endl;
    exit(EXIT_FAILURE);
  }

  while (!configFileStream.eof()) {
    // read line
    getline(configFileStream, tempString);

    // skip lines that are empty or begin with '!'
    if (!tempString.length() || *(tempString.begin()) == '!') {
      continue;
    }

    // create key value pair
    currPair = findPair(tempString, '=');

    // check that the key doesn't already exist
    if (configuration.find(currPair.first) != configuration.end()) {
      std::cerr << "Value appears twice in configuration file: "
                << currPair.first << std::endl;
      exit(EXIT_FAILURE);
    }

    // add pair
    configuration[currPair.first] = currPair.second;
  }

  return configuration;
}

std::pair<std::string, std::string> findPair(std::string inputString,
                                             char delimiter) {
  int delimiterPosition;
  std::string key;
  std::string value;

  // find the delimiter
  if ((delimiterPosition = inputString.find(delimiter)) == std::string::npos) {
    std::cerr << "Unable to find delimiter in current line of config file"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // split the string into key and value
  key = inputString.substr(0, delimiterPosition);
  value = inputString.substr(delimiterPosition + 1, std::string::npos);

  // check that neither the key nor value are empty
  if (!key.size() || !value.size()) {
    std::cerr << "Incomplete key value pair!" << std::endl;
    exit(EXIT_FAILURE);
  }

  return std::make_pair(key, value);
}