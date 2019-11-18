#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <map>
#include <string>
#include <cmath>

namespace tinyjson {
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
  char * dtoa(char *s, double n) {
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

  struct Value {
    typedef std::vector<Value> array;
    typedef std::map<std::string, Value> object;
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
    std::string to_str() const {
      switch (type) {
        case ValueType::string_type:
          return *(storage.str_val);
        case ValueType::object_type:
          return "object";
        case ValueType::array_type:
          return "array";
        case ValueType::null_type:
          return "null";
        case ValueType::number_type: {
          char buf[MAX_NUMBER_STRING_SIZE];
          return std::string(dtoa(buf, storage.num_val));
        }
        case ValueType::boolean_type:
          return storage.bool_val ? "true" : "false";
      }
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

  bool parseNumber(const char** token, double* number) {
    (*token) += strspn((*token), " \t");
    const char* end = (*token) + strcspn((*token), " \t,\n\r}");
    size_t offset = end - (*token);
    if (offset != 0) {
      char* dest = (char*)malloc(sizeof(char) * offset + 1);
      strncpy(dest, (*token), offset);
      *(dest + offset) = 0;
      double value = (double)atof(dest); // @TODO: this should be replaced with custom atod function
      (*number) = value;
      free(dest);
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

  bool parseValue(const char** token, Value& val) {
    if ((*token)[0] == '\"') {
      // string
      (*token)++;
      // empty string is not allowed
      if ((*token)[0] == '\"') return false;
      std::string str_value = parseString(token);
      // no key found
      if (str_value.empty()) return false;
      val.set_value(str_value);
    } else if ((*token)[0] == '{') {
      // object
      (*token)++;
      (*token) += strspn((*token), " \t\n\r");
    } else if ((*token)[0] == '[') {
      // array
      (*token)++;
      (*token) += strspn((*token), " \t\n\r");
    } else if (0 == strncmp((*token), "true", 4)) {
      // boolean true
      val.set_value(true);
      (*token) += 4;
    } else if (0 == strncmp((*token), "false", 5)) {
      // boolean false
      val.set_value(false);
      (*token) += 5;
    } else if (0 == strncmp((*token), "null", 4)) {
      // null
      (*token) += 4;
    } else {
      // number
      double number = 0.0;
      if (!parseNumber(token, &number)) return false;
      val.set_value(number);
    }

    if ((*token)[0] == ',') {
      (*token)++;
    }

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
        Value current_value;
        if (parseValue(&token, current_value)) {
          root.insert(std::make_pair(current_key, current_value));
        } else {
          return false;
        }
      }

      // array
      if (token[0] == '[') {
        token++;
        token += strspn(token, " \t");
        continue;
      }
    }

    value.set_value(root);
    return true;
  }
}