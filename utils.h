#ifndef UTILS_H
#define UTILS_H

#include <chrono>

class StopWatch {
public:
  void start() {
    s = std::chrono::high_resolution_clock::now();
  }
  void stop() {
    e = std::chrono::high_resolution_clock::now();
  }

  float milli() {
    return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(e - s).count();
  }

  float micro() {
    return std::chrono::duration_cast<std::chrono::duration<float, std::micro>>(e - s).count();
  }

  float nano() {
    return std::chrono::duration_cast<std::chrono::duration<float, std::nano>>(e - s).count();
  }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> s;
  std::chrono::time_point<std::chrono::high_resolution_clock> e;
};

#endif // UTILS_H
