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
  json_node arrValue({json_node(32.0), json_node(75.0), json_node(99.0)});
  obj.insert(std::make_pair("arr", arrValue));
  json_node newValue(obj);

  std::cout << newValue.serialize(true) << std::endl;
  std::cout << std::boolalpha << (node == newValue) << std::endl;

  if (newValue.is<object>()) {
   json_node& val = newValue.get_node("arr").get_element(0);
   std::cout << val.serialize() << std::endl;
   std::cout << "+===================" << std::endl;

//   json_node& val2 = newValue.get_node("empty");
   object empty_value;
   if (newValue.get(empty_value)) {
     for (auto& v : empty_value) {
       std::cout << v.second.serialize(true) << std::endl;
     }
   }
  }

  return 0;
}