#include "tinyjson.h"
#include <fstream>

int main() {
  tinyjson::Value value;
  bool res = parseJsonFromFile(value, "../sample3.json");
  if (!res) {
    std::cout << "parse failed" << '\n';
  } else {
    std::cout << value.pretty() << std::endl;
  }

  return 0;
}