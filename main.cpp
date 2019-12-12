#include "tinyjson.h"
#include "utils.h"

using namespace tinyjson;

int main() {
  JsonValue value;
  std::string json;
  bool res = read_file("../sample8.json", json);
  if (!res) {
    std::cout << "file not found!" << std::endl;
    return -1;
  }

  StopWatch watch;
  watch.start();
  res = parse(value, json);
  watch.stop();

  float elapsed = watch.milli();
  if (!res) {
    std::cout << "parse failed" << '\n';
  } else {
    JsonValue newValue;
    object obj;
    JsonValue strVal;
    strVal.set_value("value");
    obj.insert(std::make_pair("key1", strVal));
    object obj2;
    JsonValue strVal2;
    strVal2.set_value("min123");
    obj2.insert(std::make_pair("name", strVal2));
    JsonValue newValue2;
    newValue2.set_value(obj2);
    obj.insert(std::make_pair("empty", newValue2));
    newValue.set_value(obj);

    std::cout << "time elapsed: " << elapsed << " ms" << std::endl;
    std::cout << value.print(true) << std::endl;
//
    std::cout << newValue.print(true) << std::endl;
    std::cout << std::boolalpha << (value == newValue) << std::endl;
    std::cout << newValue.print(true) << std::endl;
  }

  return 0;
}