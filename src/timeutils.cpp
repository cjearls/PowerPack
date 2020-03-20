#include "timeutils.h"

uint64_t millis() {
  uint64_t ms =
      std::chrono::duration_cast<std::chrono::duration<uint64_t, std::milli>>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return ms;
}

uint64_t micros() {
  uint64_t us =
      std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return us;
}

uint64_t nanos() {
  uint64_t ns =
      std::chrono::duration_cast<std::chrono::duration<uint64_t, std::nano>>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return ns;
}
