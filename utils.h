#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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

#define BUF_SIZE 8192

void handle_error(const char* msg) {
  perror(msg);
  exit(255);
}

bool mmap_file_read(const std::string& path, std::string& output) {
  size_t len;
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1)
    return false;

  struct stat sb;
  if (fstat(fd, &sb) == -1)
    return false;

  len = sb.st_size;

  const char* addr = (const char*)(mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0u));
  if (addr == MAP_FAILED)
    return false;

  output.assign(addr, len);

  int res = munmap((void*)addr, len);
  if (res == -1)
    return false;

  close(fd);

  return true;
}

#endif // UTILS_H
