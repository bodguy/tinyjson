#include "tinyjson.h"
#include "utils.h"

using namespace tinyjson;

int main() {
  json_node node;
  std::string json;
  bool res = read_file("../sample8.json", json);
  if (!res) {
    std::cout << "file not found!" << std::endl;
    return -1;
  }

  StopWatch watch;
  watch.start();
  res = deserialize(node, json);
  watch.stop();

  if (!res) {
    std::cout << "json parse failed" << std::endl;
    return -1;
  }

  std::cout << "time elapsed: " << watch.milli() << " ms" << std::endl;
  std::cout << node.serialize(true) << std::endl;

  // make a new json
  object obj;
  obj.insert(std::make_pair("key1", json_node("value")));
  object obj2;
  obj2.insert(std::make_pair("name", json_node("min123")));
  obj.insert(std::make_pair("empty", json_node(obj2)));
  json_node arrValue({json_node(1.0), json_node(2.0), json_node(3.0)});
  obj.insert(std::make_pair("arr", arrValue));
  json_node newValue(obj);

  std::cout << newValue.serialize(true) << std::endl;
  std::cout << std::boolalpha << (node == newValue) << std::endl;

  return 0;
}