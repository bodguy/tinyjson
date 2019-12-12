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
    std::cout << "json parse failed" << std::endl;
    return -1;
  }

  object obj;
  obj.insert(std::make_pair("key1", JsonValue("value")));
  object obj2;
  obj2.insert(std::make_pair("name", JsonValue("min123")));
  obj.insert(std::make_pair("empty", JsonValue(obj2)));
  JsonValue arrValue({JsonValue(1.0), JsonValue(2.0), JsonValue(3.0)});
  obj.insert(std::make_pair("arr", arrValue));
  JsonValue newValue(obj);

  std::cout << "time elapsed: " << elapsed << " ms" << std::endl;
  std::cout << value.print(true) << std::endl;
  std::cout << newValue.print(true) << std::endl;
  std::cout << std::boolalpha << (value == newValue) << std::endl;

  return 0;
}