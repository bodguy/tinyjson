#include "tinyjson.h"
#include "utils.h"

int main() {
  tinyjson::Value value;
  StopWatch watch;
  std::string json;
  bool res = tinyjson::read_file("../sample7.json", json);
  if (!res) return -1;
  watch.start();
  res = tinyjson::parseJson(value, json);
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