#include "tinyjson.h"
#include "utils.h"

int main() {
  tinyjson::Value value;
  StopWatch watch;
  watch.start();
  bool res = parseJsonFromFile(value, "../sample2.json");
  watch.stop();
  float elapsed = watch.milli();
  if (!res) {
    std::cout << "parse failed" << '\n';
  } else {
    std::cout << "time elapsed: " << elapsed << std::endl;
    std::cout << value.pretty() << std::endl;
  }

  return 0;
}