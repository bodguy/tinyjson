#include <iostream>
#include <cstring>
#include <map>

struct Value {
  Value() :value_map() {}
  std::map<std::string, int> value_map;
};

bool parseValue(const char** token, int* result) {
  (*token) += strspn((*token), " \t");
  const char* end = (*token) + strcspn((*token), " \t,}");
  size_t offset = end - (*token);
  if (offset != 0) {
    char* dest = (char*)malloc(sizeof(char) * offset + 1);
    strncpy(dest, (*token), offset);
    *(dest + offset) = 0;
    int value = atoi(dest);
    (*result) = value;
    free(dest);
    (*token) = end;
    return true;
  }

  return false;
}

std::string parseKey(const char** token, const char* endKey) {
  (*token) += strspn((*token), " \t");
  const char* end = (*token) + strcspn((*token), endKey);
  size_t offset = end - (*token);
  std::string key;
  if (offset != 0) {
    char* dest = (char*)malloc(sizeof(char) * offset + 1);
    strncpy(dest, (*token), offset);
    *(dest + offset) = 0;
    key.assign(dest);
    free(dest);
  }

  (*token) = ++end;
  return key;
}

bool parseJson(Value& value, const std::string& json) {
  const char* token = json.c_str();
  bool is_start = false;

  while (token) {
    token += strspn(token, " \t");

    if (token == nullptr) return false;

    // start of object
    if (token[0] == '{') {
      is_start = true;
      token++;
      token += strspn(token, " \t");
      // empty {} json
      if (token[0] == '}') return true;
      std::string key;
      if (token[0] == '\"') {
        token++;
        key = parseKey(&token, "\"");
      } else if (token[0] == '\'') {
        token++;
        key = parseKey(&token, "\'");
      }
      // no key detect
      if (key.empty()) return false;
      token += strspn(token, " \t");
      if (token[0] == ':') {
        token++;
        int result = 0;
        if(parseValue(&token, &result)) {
          value.value_map.insert(std::make_pair(key, result));
        }
      }
      continue;
    }

    // end of json
    if (token[0] == '}') {
      return is_start;
    }

    // array
    if (token[0] == '[') {
      token += strspn(token, " \t");
      continue;
    }
  }

  return true;
}

int main() {
  Value v;
  std::string json_str = R"JSON(   { "   fsdfsafasdfasdfsdf": 1234444    })JSON";
  bool res = parseJson(v, json_str);
  if (!res) {
    std::cout << "parse failed" << '\n';
  } else {
    std::map<std::string, int>::const_iterator iter;
    for (iter = v.value_map.begin(); iter != v.value_map.end(); iter++) {
      printf("%s : %d\n", iter->first.c_str(), iter->second);
    }
  }

  return 0;
}