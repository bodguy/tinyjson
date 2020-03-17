#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <fstream>

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

bool read_file(const std::string& path, std::string& output) {
  std::ifstream ifs(path);
  if(!ifs) {
    return false;
  }

  ifs.seekg(0, std::ios::end);
  output.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  output.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  return true;
}

#endif // UTILS_H
