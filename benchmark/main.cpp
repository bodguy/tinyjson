#include "utils.h"
#include <tinyjson.h>

using namespace tinyjson;

int main() {
  StopWatch watch;
  json_node node;
  std::string json;

  watch.start();
  bool res = read_file("../sample/sample14.json", json);
  watch.stop();

  if (!res) {
    std::cout << "file not found!" << std::endl;
    return -1;
  }

  std::cout << "read from file elapsed: " << watch.milli() << " ms" << std::endl;

  std::string err;
  watch.start();
  res = json_parser::parse(node, json, err);
  watch.stop();

  if (!res) {
    std::cout << err << std::endl;
    return -1;
  }

  std::cout << "parsing json elapsed: " << watch.milli() << " ms" << std::endl;

  watch.start();
  std::string serialized = node.serialize(true);
  watch.stop();

  std::cout << "serialize json elapsed: " << watch.milli() << " ms" << std::endl;
  std::cout << serialized << std::endl;

  return 0;
}
