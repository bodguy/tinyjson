#include "tinyjson.h"
#include "utils.h"

using namespace tinyjson;

int main() {
  json_node node;
  std::string json;
  bool res = read_file("../sample/sample12.json", json);
  if (!res) {
    std::cout << "file not found!" << std::endl;
    return -1;
  }

  res = parse(node, json);

  if (!res) {
    std::cout << "json parse failed" << std::endl;
    return -1;
  }

  if (node.is<array>()) {
    json_node n = node.get_element(0).get_node("name");
    std::cout << n.serialize(true) << std::endl;
  }

  return 0;
}