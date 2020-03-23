#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/**
 * Base class that specifies responses to measurement events
 */
class eventHandler {
 public:
  std::fstream writer;
  // TODO: IS IT NECESSARY TO KEEP TRACK OF THE LOG FILE?
  std::string logFile;

  // constructor
  eventHandler();

  // executed when a "start session" communication is received
  virtual void startHandler(uint64_t timestamp) = 0;

  // executed when a "tag" communication is recieved
  virtual void tagHandler(uint64_t timestamp, std::string tag) = 0;

  // executed when a "end session" communication is received
  virtual void endHandler(uint64_t timestamp) = 0;

  // destructor
  virtual ~eventHandler();

 protected:
  // This stores a list of timestamps and their identifying strings.
  std::vector<std::pair<std::string, uint64_t>> timestamps;
};

#endif