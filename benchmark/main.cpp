#include "../tinyjson.h"
#include "utils.h"

using namespace tinyjson;

int main() {
  StopWatch watch;
  json_node node;
  std::string json;

  watch.start();
  bool res = read_file_fast("../sample/sample8.json", json);
  watch.stop();

  if (!res) {
    std::cout << "file not found!" << std::endl;
    return -1;
  }

  std::cout << "read from file elapsed: " << watch.milli() << " ms" << std::endl;

  watch.start();
  res = parse(node, json);
  watch.stop();

  if (!res) {
    std::cout << "json parse failed" << std::endl;
    return -1;
  }

  std::cout << "parsing json elapsed: " << watch.milli() << " ms" << std::endl;

  watch.start();
  std::string serialized = node.serialize(true);
  watch.stop();

  std::cout << "serialize json elapsed: " << watch.milli() << " ms" << std::endl;

  return 0;
}