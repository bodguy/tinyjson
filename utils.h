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

bool read_file_fast(const std::string& path, std::string& output) {
  static const size_t BUF_SIZE = 8192;
  int fd = open(path.c_str(), O_RDONLY);
  if(fd == -1)
    return false;

  // apple computer does not implement fadvise
#ifndef __APPLE__
  posix_fadvise(fd, 0, 0, 1);
#endif

  char buf[BUF_SIZE + 1];
  while(size_t bytes_read = read(fd, buf, BUF_SIZE)) {
    if(bytes_read == (size_t)-1)
      return false;
    if (!bytes_read)
      break;
    output.append(buf);
  }

  close(fd);

  return true;
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
