#include "tinyjson.h"
#include <fstream>

// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
bool read_json_from_file(const std::string& path, std::string& json_str) {
  std::ifstream ifs(path);
  if(!ifs) {
    return false;
  }

  ifs.seekg(0, std::ios::end);
  json_str.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  json_str.assign((std::istreambuf_iterator<char>(ifs)),
             std::istreambuf_iterator<char>());
  return true;
}

int main() {
  tinyjson::Value val;
  std::string json_str;
  read_json_from_file("../sample2.json", json_str);
  bool res = parseJson(val, json_str);
  if (!res) {
    std::cout << "parse failed" << '\n';
  } else {
    for (auto iter : *(val.storage.object_val)) {
      std::cout << iter.first << " : " << iter.second.to_str() << std::endl;
    }
  }

  return 0;
}