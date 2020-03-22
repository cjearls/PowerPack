#include "timeutils.h"

/**
 * Returns the number of milliseconds since jan 1, 1970
 * 
 * @returns the number of milliseconds since jan 1, 1970
*/
uint64_t millis() {
  uint64_t ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return ms;
}

/**
 * Returns the number of microseconds since jan 1, 1970
 * 
 * @returns the number of microseconds since jan 1, 1970
*/
uint64_t micros() {
  uint64_t us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return us;
}

/**
 * Returns the number of nanoseconds since jan 1, 1970
 * 
 * @returns the number of nanoseconds since jan 1, 1970
*/
uint64_t nanos() {
  uint64_t ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return ns;
}
