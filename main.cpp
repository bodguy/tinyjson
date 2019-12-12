#include "tinyjson.h"
#include "utils.h"

int main() {
  tinyjson::JsonValue value;
  std::string json;
  bool res = read_file("../sample7.json", json);
  if (!res) {
    std::cout << "file not found!" << std::endl;
    return -1;
  }

  StopWatch watch;
  watch.start();
  res = tinyjson::parse(value, json);
  watch.stop();

  float elapsed = watch.milli();
  if (!res) {
    std::cout << "parse failed" << '\n';
  } else {
    std::cout << "time elapsed: " << elapsed << " ms" << std::endl;
    std::cout << value.print(true) << std::endl;
  }

  return 0;
}