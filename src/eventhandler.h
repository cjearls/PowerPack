#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

class eventHandler {
 public:
  eventHandler();
  virtual void startHandler(uint64_t timestamp) = 0;
  virtual void tagHandler(uint64_t timestamp, std::string tag) = 0;
  virtual void endHandler(uint64_t timestamp) = 0;
  virtual ~eventHandler();

  private:
    std::fstream writer;
    std::string logFile; //TODO: IS IT NECESSARY TO KEEP TRACK OF THE LOG FILE?

    // This stores a list of timestamps and their identifying strings.
    std::vector<std::pair<std::string, uint64_t>> timestamps;
};

#endif