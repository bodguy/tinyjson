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

  res = deserialize(node, json);

  if (!res) {
    std::cout << "json parse failed" << std::endl;
    return -1;
  }

  std::cout << node.serialize(true) << std::endl;

  return 0;
}