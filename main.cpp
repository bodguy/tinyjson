#include <iostream>
#include <cstring>
#include <vector>
#include <map>

enum class ValueType {
  null_type = 0,
  boolean_type,
  number_type,
  string_type,
  array_type,
  object_type
};

struct Value {
  typedef std::vector<Value> array;
  typedef std::map<std::string, Value> object;
  union {
    bool bool_val;
    int num_val;
    std::string str_val;
    array array_val;
    object object_val;
  };
  ValueType type;
};

struct Result {
  Result() : value_map() {}
  std::map<std::string, int> value_map;
};

bool parseValue(const char** token, int* result) {
  (*token) += strspn((*token), " \t");
  const char* end = (*token) + strcspn((*token), " \t,\n\r}");
  size_t offset = end - (*token);
  if (offset != 0) {
    char* dest = (char*)malloc(sizeof(char) * offset + 1);
    strncpy(dest, (*token), offset);
    *(dest + offset) = 0;
    int value = atoi(dest);
    (*result) = value;
    free(dest);
    if (end[0] == ',') {
      end++;
    }
    (*token) = end;
    return true;
  }

  return false;
}

std::string parseKey(const char** token) {
  const char* end = (*token) + strcspn((*token), "\"");
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

bool parseJson(Result& value, const std::string& json) {
  const char* token = json.c_str();
  bool start_of_object = false;

  while ((*token)) {
    token += strspn(token, " \t\n\r");

    if (token == nullptr) return false;

    // start of object
    if (token[0] == '{') {
      start_of_object = true;
      token++;
      token += strspn(token, " \t\n\r");
      // empty {} json
      if (token[0] == '}') return true;
      std::string key;
      if (token[0] == '\"') {
        token++;
        key = parseKey(&token);
      }
      // no key detect
      if (key.empty()) return false;
      token += strspn(token, " \t\n\r");
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
      if (start_of_object) {
        start_of_object = !start_of_object;
        token++;
      } else {
        return false;
      }
      continue;
    }

    // start of key
    if (token[0] == '\"') {
      token++;
      // empty string is not allowed
      if (token[0] == '\"') return false;
      std::string key;
      key = parseKey(&token);
      // no key detect
      if (key.empty()) return false;
      token += strspn(token, " \t\n\r");
      if (token[0] == ':') {
        token++;
        int result = 0;
        if(parseValue(&token, &result)) {
          value.value_map.insert(std::make_pair(key, result));
        }
      }
      continue;
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
  Result v;
  std::string json_str = R"JSON({
"number": 123,
"name": 999,
"person": 33,
"nickname": 1,
})JSON";
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