#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <unordered_map>
#include <list>
#include <string>
#include <cmath>
#include <sstream>
#include <fstream>

#define IS_DIGIT(x) \
  (static_cast<unsigned int>((x) - '0') < static_cast<unsigned int>(10))

namespace tinyjson {
  // https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
  bool read_file(const std::string& path, std::string& json_str) {
    std::ifstream ifs(path);
    if(!ifs) {
      return false;
    }

    ifs.seekg(0, std::ios::end);
    json_str.reserve(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    json_str.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return true;
  }

  template <typename K, typename V>
  class linked_hash_map {
  public:
    typedef std::pair<K, V> value_type;
    typedef int size_type;
    typedef typename std::list<value_type>::iterator iterator;
    typedef typename std::list<value_type>::const_iterator const_iterator;

  public:
    linked_hash_map() : linked_list(), hash_map() {
      linked_list.clear();
      hash_map.clear();
    }

    auto insert(const value_type& value) {
      linked_list.push_back(value);
      return hash_map.insert(std::make_pair(value.first, std::prev(linked_list.end())));
    }

    bool erase(const K& key) {
      typename std::unordered_map<K, iterator>::iterator iter = hash_map.find(key);
      if (iter == hash_map.end()) {
        return false;
      }
      linked_list.erase((*iter).second);
      hash_map.erase(iter);
      return true;
    }

    size_type size() const {
      return linked_list.size();
    }

    bool empty() const {
      return linked_list.empty();
    }

    iterator begin() { return linked_list.begin(); }
    iterator end() { return linked_list.end(); }
    const_iterator cbegin() const { return linked_list.cbegin(); }
    const_iterator cend() const { return linked_list.cend(); }

  private:
    std::list<value_type> linked_list;
    std::unordered_map<K, iterator> hash_map;
  };

  enum class ValueType {
    null_type = 0,
    boolean_type,
    number_type,
    string_type,
    array_type,
    object_type
  };

  // https://stackoverflow.com/questions/2302969/convert-a-float-to-a-string
  static double PRECISION = 0.00000000000001;
  static int MAX_NUMBER_STRING_SIZE = 32;
  static char * dtoa(char *s, double n) {
    // handle special cases
    if (std::isnan(n)) {
      strcpy(s, "nan");
    } else if (std::isinf(n)) {
      strcpy(s, "inf");
    } else if (n == 0.0) {
      strcpy(s, "0");
    } else {
      int digit, m, m1;
      char *c = s;
      int neg = (n < 0);
      if (neg)
        n = -n;
      // calculate magnitude
      m = log10(n);
      int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
      if (neg)
        *(c++) = '-';
      // set up for scientific notation
      if (useExp) {
        if (m < 0)
          m -= 1.0;
        n = n / pow(10.0, m);
        m1 = m;
        m = 0;
      }
      if (m < 1.0) {
        m = 0;
      }
      // convert the number
      while (n > PRECISION || m >= 0) {
        double weight = pow(10.0, m);
        if (weight > 0 && !std::isinf(weight)) {
          digit = floor(n / weight);
          n -= (digit * weight);
          *(c++) = '0' + digit;
        }
        if (m == 0 && n > 0)
          *(c++) = '.';
        m--;
      }
      if (useExp) {
        // convert the exponent
        int i, j;
        *(c++) = 'e';
        if (m1 > 0) {
          *(c++) = '+';
        } else {
          *(c++) = '-';
          m1 = -m1;
        }
        m = 0;
        while (m1 > 0) {
          *(c++) = '0' + m1 % 10;
          m1 /= 10;
          m++;
        }
        c -= m;
        for (i = 0, j = m-1; i<j; i++, j--) {
          // swap without temporary
          c[i] ^= c[j];
          c[j] ^= c[i];
          c[i] ^= c[j];
        }
        c += m;
      }
      *(c) = '\0';
    }
    return s;
  }

  // https://github.com/syoyo/tinyobjloader/blob/master/tiny_obj_loader.h
  static bool atod(const char *s, const char *s_end, double *result) {
    if (s >= s_end) {
      return false;
    }

    double mantissa = 0.0;
    // This exponent is base 2 rather than 10.
    // However the exponent we parse is supposed to be one of ten,
    // thus we must take care to convert the exponent/and or the
    // mantissa to a * 2^E, where a is the mantissa and E is the
    // exponent.
    // To get the final double we will use ldexp, it requires the
    // exponent to be in base 2.
    int exponent = 0;

    // NOTE: THESE MUST BE DECLARED HERE SINCE WE ARE NOT ALLOWED
    // TO JUMP OVER DEFINITIONS.
    char sign = '+';
    char exp_sign = '+';
    char const *curr = s;

    // How many characters were read in a loop.
    int read = 0;
    // Tells whether a loop terminated due to reaching s_end.
    bool end_not_reached = false;
    bool leading_decimal_dots = false;

    /*
            BEGIN PARSING.
    */

    // Find out what sign we've got.
    if (*curr == '+' || *curr == '-') {
      sign = *curr;
      curr++;
      if ((curr != s_end) && (*curr == '.')) {
        // accept. Somethig like `.7e+2`, `-.5234`
        leading_decimal_dots = true;
      }
    } else if (IS_DIGIT(*curr)) { /* Pass through. */
    } else if (*curr == '.') {
      // accept. Somethig like `.7e+2`, `-.5234`
      leading_decimal_dots = true;
    } else {
      goto fail;
    }

    // Read the integer part.
    end_not_reached = (curr != s_end);
    if (!leading_decimal_dots) {
      while (end_not_reached && IS_DIGIT(*curr)) {
        mantissa *= 10;
        mantissa += static_cast<int>(*curr - 0x30);
        curr++;
        read++;
        end_not_reached = (curr != s_end);
      }

      // We must make sure we actually got something.
      if (read == 0) goto fail;
    }

    // We allow numbers of form "#", "###" etc.
    if (!end_not_reached) goto assemble;

    // Read the decimal part.
    if (*curr == '.') {
      curr++;
      read = 1;
      end_not_reached = (curr != s_end);
      while (end_not_reached && IS_DIGIT(*curr)) {
        static const double pow_lut[] = {
                1.0, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001,
        };
        const int lut_entries = sizeof pow_lut / sizeof pow_lut[0];

        // NOTE: Don't use powf here, it will absolutely murder precision.
        mantissa += static_cast<int>(*curr - 0x30) *
                    (read < lut_entries ? pow_lut[read] : std::pow(10.0, -read));
        read++;
        curr++;
        end_not_reached = (curr != s_end);
      }
    } else if (*curr == 'e' || *curr == 'E') {
    } else {
      goto assemble;
    }

    if (!end_not_reached) goto assemble;

    // Read the exponent part.
    if (*curr == 'e' || *curr == 'E') {
      curr++;
      // Figure out if a sign is present and if it is.
      end_not_reached = (curr != s_end);
      if (end_not_reached && (*curr == '+' || *curr == '-')) {
        exp_sign = *curr;
        curr++;
      } else if (IS_DIGIT(*curr)) { /* Pass through. */
      } else {
        // Empty E is not allowed.
        goto fail;
      }

      read = 0;
      end_not_reached = (curr != s_end);
      while (end_not_reached && IS_DIGIT(*curr)) {
        exponent *= 10;
        exponent += static_cast<int>(*curr - 0x30);
        curr++;
        read++;
        end_not_reached = (curr != s_end);
      }
      exponent *= (exp_sign == '+' ? 1 : -1);
      if (read == 0) goto fail;
    }

    assemble:
    *result = (sign == '+' ? 1 : -1) *
              (exponent ? std::ldexp(mantissa * std::pow(5.0, exponent), exponent)
                        : mantissa);
    return true;
    fail:
    return false;
  }

  struct Value {
    typedef std::vector<Value> array;
    typedef linked_hash_map<std::string, Value> object;
    Value() : storage(), type(ValueType::null_type) {}
    ~Value() {
//      switch (type) {
//        case ValueType::string_type:
//          delete storage.str_val;
//          storage.str_val = nullptr;
//          break;
//        case ValueType::array_type:
//          delete storage.array_val;
//          storage.array_val = nullptr;
//          break;
//        case ValueType::object_type:
//          delete storage.object_val;
//          storage.object_val = nullptr;
//          break;
//        default:
//          break;
//      }
    }
    void set_value(bool val) { type = ValueType::boolean_type; storage.bool_val = val; }
    void set_value(double val) { type = ValueType::number_type; storage.num_val = val; }
    void set_value(const std::string& val) { type = ValueType::string_type; storage.str_val = new std::string(val); }
    void set_value(const array& val) { type = ValueType::array_type; storage.array_val = new array(val); }
    void set_value(const object& val) { type = ValueType::object_type; storage.object_val = new object(val); }
    std::string to_type() const {
      switch (type) {
        case ValueType::string_type:
          return "string";
        case ValueType::object_type:
          return "object";
        case ValueType::array_type:
          return "array";
        case ValueType::null_type:
          return "null";
        case ValueType::number_type:
          return "number";
        case ValueType::boolean_type:
          return "boolean";
      }
    }

    static std::string indentation(unsigned int indent) {
      std::stringstream sstream;
      for (int i = 0; i < indent; i++) {
        sstream << "  ";
      }
      return sstream.str();
    }

    std::string write(bool pretty = false) {
      if (pretty) {
        return "{\n" + pretty_print() + "\n}";
      } else {
        return "{" + print() + "}";
      }
    }

    std::string print(bool has_next = true) const {
      std::stringstream sstream;
      std::string delim;

      if (has_next) {
        delim = ',';
      }

      switch (type) {
        case ValueType::string_type:
          sstream << '\"' << *(storage.str_val) << '\"' << delim;
          break;
        case ValueType::object_type: {
          for (auto iter = storage.object_val->cbegin(); iter != storage.object_val->cend(); iter++) {
            sstream << "\"" << iter->first << "\":";
            if (iter->second.type == ValueType::object_type) {
              sstream << '{';
            }
            auto iterCopy = iter;
            bool next = false;
            if (++iterCopy != storage.object_val->cend()) {
              next = true;
            }
            sstream << iter->second.print(next);
            if (iter->second.type == ValueType::object_type) {
              sstream << '}' << (next ? "," : "");
            }
          }
          break;
        }
        case ValueType::array_type:
          sstream << "array" << delim;
          break;
        case ValueType::null_type:
          sstream << "null" << delim;
          break;
        case ValueType::number_type: {
          char buf[MAX_NUMBER_STRING_SIZE];
          sstream << std::string(dtoa(buf, storage.num_val)) << delim;
          break;
        }
        case ValueType::boolean_type:
          sstream << (storage.bool_val ? "true" : "false") << delim;
          break;
      }

      return sstream.str();
    }

    std::string pretty_print(unsigned int indent = 1, bool has_next = true) const {
      std::stringstream sstream;
      std::string delim;

      if (has_next) {
        delim = ',';
      }

      switch (type) {
        case ValueType::string_type:
          sstream << '\"' << *(storage.str_val) << '\"' << delim;
          break;
        case ValueType::object_type: {
          std::string space = indentation(indent);
          for (auto iter = storage.object_val->cbegin(); iter != storage.object_val->cend(); iter++) {
            // prevent first newline
            if (iter != storage.object_val->begin()) {
              sstream << '\n';
            }
            sstream << space << "\"" << iter->first << "\": ";
            if (iter->second.type == ValueType::object_type) {
              sstream << '{';
              if (!iter->second.storage.object_val->empty()) {
                sstream << '\n';
              }
            }
            auto iterCopy = iter;
            bool next = false;
            if (++iterCopy != storage.object_val->cend()) {
              next = true;
            }
            sstream << iter->second.pretty_print(indent + 1, next);
            if (iter->second.type == ValueType::object_type) {
              if (!iter->second.storage.object_val->empty()) {
                sstream << '\n' << space;
              }
              sstream << '}' << (next ? "," : "");
            }
          }
          break;
        }
        case ValueType::array_type: {
          std::string space = indentation(indent);
          for (auto iter = storage.array_val->cbegin(); iter != storage.array_val->cend(); iter++) {
            sstream << "array" << delim;
          }
          break;
        }
        case ValueType::null_type:
          sstream << "null" << delim;
          break;
        case ValueType::number_type: {
          char buf[MAX_NUMBER_STRING_SIZE];
          sstream << std::string(dtoa(buf, storage.num_val)) << delim;
          break;
        }
        case ValueType::boolean_type:
          sstream << (storage.bool_val ? "true" : "false") << delim;
          break;
      }

      return sstream.str();
    }

    union Storage {
      bool bool_val;
      double num_val;
      std::string* str_val;
      array* array_val;
      object* object_val;
    };
    Storage storage;
    ValueType type;
  };

  typedef Value::array array;
  typedef Value::object object;

  bool parseNumber(const char** token, double* number);
  std::string parseString(const char** token);
  bool parseValue(Value& value, const char** token);
  bool parseObject(Value& value, const char** token);
  bool parseArray(Value& value, const char** token);

  bool parseNumber(const char** token, double* number) {
    (*token) += strspn((*token), " \t");
    const char* end = (*token) + strcspn((*token), " \t,\n\r}");
    if (end != (*token)) {
      double value;
      if (!atod((*token), end, &value)) return false;
      (*number) = value;
      if (end[0] == ',') {
        end++;
      }
      (*token) = end;
      return true;
    }

    return false;
  }

  std::string parseString(const char** token) {
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

  bool parseValue(Value& value, const char** token) {
    if ((*token)[0] == '\"') {
      // string
      (*token)++;
      // empty string is not allowed
      if ((*token)[0] == '\"') return false;
      std::string str_value = parseString(token);
      value.set_value(str_value);
    } else if (0 == strncmp((*token), "true", 4)) {
      // boolean true
      value.set_value(true);
      (*token) += 4;
    } else if (0 == strncmp((*token), "false", 5)) {
      // boolean false
      value.set_value(false);
      (*token) += 5;
    } else if (0 == strncmp((*token), "null", 4)) {
      // null
      (*token) += 4;
    } else {
      // number
      double number = 0.0;
      if (!parseNumber(token, &number)) return false;
      value.set_value(number);
    }

    if ((*token)[0] == ',') {
      (*token)++;
    }

    return true;
  }

  bool parseObject(Value& value, const char** token) {
    std::string current_key;
    object root;

    while ((*token)[0]) {
      (*token) += strspn((*token), " \t\n\r");

      if ((*token) == nullptr) return false;

      // end of object
      if ((*token)[0] == '}') {
        (*token)++;
        break;
      }

      // start of key
      if ((*token)[0] == '\"') {
        (*token)++;
        // empty string is not allowed
        if ((*token)[0] == '\"') return false;
        current_key = parseString(token);
        // no key found
        if (current_key.empty()) return false;
        if ((*token)[0] == ',') token++;
        continue;
      }

      // start of value
      if ((*token)[0] == ':') {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");
        Value current_value;
        // another object
        if ((*token)[0] == '{') {
          (*token)++;
          if (parseObject(current_value, token)) {
            root.insert(std::make_pair(current_key, current_value));
          } else {
            return false;
          }
        } else {
          if (parseValue(current_value, token)) {
            root.insert(std::make_pair(current_key, current_value));
          } else {
            return false;
          }
        }
        continue;
      }
    }

    if ((*token)[0] == ',') {
      (*token)++;
    }

    value.set_value(root);
    return true;
  }

  bool parseArray(Value& value, const char** token) {
    array root;

    while ((*token)[0]) {
      (*token) += strspn((*token), " \t\n\r");

      if ((*token) == nullptr) return false;

      // end of array
      if ((*token)[0] == ']') {
        (*token)++;
        break;
      }

      if ((*token)[0] == ',') {
        (*token)++;
      }

      if ((*token)[0] == '{') {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");
        Value current_obj;
        if (parseObject(current_obj, token)) {
          root.emplace_back(current_obj);
        } else {
          return false;
        }
        continue;
      } else if ((*token)[0] == '[') {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");
        Value current_arr;
        if (parseArray(current_arr, token)) {
          root.emplace_back(current_arr);
        } else {
          return false;
        }
        continue;
      } else {
        (*token) += strspn((*token), " \t\n\r");
        Value current_value;
        if (parseValue(current_value, token)) {
          root.push_back(current_value);
        } else {
          return false;
        }
        continue;
      }
    }

    if ((*token)[0] == ',') {
      (*token)++;
    }

    value.set_value(root);
    return true;
  }

  bool parseJson(Value& value, const std::string& json) {
    const char* token = json.c_str();
    bool start_of_object = false;
    std::string current_key;
    object root;

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
        current_key = parseString(&token);
        // no key found
        if (current_key.empty()) return false;
        if (token[0] == ',') token++;
        continue;
      }

      // start of value
      if (token[0] == ':' && start_of_object) {
        token++;
        token += strspn(token, " \t\n\r");

        if (token[0] == '{') {
          token++;
          Value obj_val;
          if (!parseObject(obj_val, &token))
            return false;
          root.insert(std::make_pair(current_key, obj_val));
        } else if (token[0] == '[') {
          token++;
          Value array_val;
          if (!parseArray(array_val, &token))
            return false;
          root.insert(std::make_pair(current_key, array_val));
        } else {
          Value current_value;
          if (!parseValue(current_value, &token))
            return false;
          root.insert(std::make_pair(current_key, current_value));
        }

        continue;
      }
    }

    value.set_value(root);
    return true;
  }

  bool parseJsonFromFile(Value& value, const std::string& path) {
    std::string json_str;
    bool res = read_file(path, json_str);
    if (res) {
      return parseJson(value, json_str);
    }

    return false;
  }
}