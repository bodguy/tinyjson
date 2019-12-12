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

// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
bool read_file(const std::string& path, std::string& json_str) {
  std::ifstream ifs(path);
  if(!ifs) {
    return false;
  }

  ifs.seekg(0, std::ios::end);
  json_str.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  json_str.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  return true;
}

#endif // UTILS_H
