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

namespace tinyjson {
  const double dbl_epsilon = std::numeric_limits<double>::epsilon();
  const unsigned int indent_size = 2;
  constexpr bool is_digit(const char x) {
    return static_cast<unsigned int>((x) - '0') < static_cast<unsigned int>(10);
  }

  inline bool is_equal(const double a, const double b) {
    return std::fabs(a - b) < dbl_epsilon;
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

  enum class TokenType {
    start_object = '{',
    end_object = '}',
    start_array = '[',
    end_array = ']',
    double_quote = '\"',
    start_value = ':',
    comma_separator = ','
  };

  inline bool operator==(const char a, const TokenType b) {
    return static_cast<TokenType>(a) == b;
  }

  inline bool operator!=(const char a, const TokenType b) {
    return static_cast<TokenType>(a) != b;
  }

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
    } else if (is_digit(*curr)) { /* Pass through. */
    } else if (*curr == '.') {
      // accept. Somethig like `.7e+2`, `-.5234`
      leading_decimal_dots = true;
    } else {
      goto fail;
    }

    // Read the integer part.
    end_not_reached = (curr != s_end);
    if (!leading_decimal_dots) {
      while (end_not_reached && is_digit(*curr)) {
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
      while (end_not_reached && is_digit(*curr)) {
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
      } else if (is_digit(*curr)) { /* Pass through. */
      } else {
        // Empty E is not allowed.
        goto fail;
      }

      read = 0;
      end_not_reached = (curr != s_end);
      while (end_not_reached && is_digit(*curr)) {
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

  class JsonValue {
  public:
    typedef std::vector<JsonValue> array;
    typedef linked_hash_map<std::string, JsonValue> object;
    typedef bool boolean;
    typedef double number;
    typedef std::string string;
    union Storage {
      boolean bool_val;
      number num_val;
      string* str_val;
      array* array_val;
      object* object_val;
    };

    inline JsonValue() : storage(), type(ValueType::null_type) {}
    inline ~JsonValue() {
      switch (type) {
        case ValueType::string_type:
//          delete storage.str_val;
//          printf("String destruct\n");
          break;
        case ValueType::array_type:
//          delete storage.array_val;
//          printf("Array destruct\n");
          break;
        case ValueType::object_type:
//          delete storage.object_val;
//          printf("Object destruct\n");
          break;
        default:
          break;
      }
    }
    inline void set_value(boolean val) { type = ValueType::boolean_type; storage.bool_val = val; }
    inline void set_value(number val) { type = ValueType::number_type; storage.num_val = val; }
    inline void set_value(const string& val) { type = ValueType::string_type; storage.str_val = new std::string(val); }
    inline void set_value(const char* val) { type = ValueType::string_type; storage.str_val = new std::string(val); }
    inline void set_value(const array& val) { type = ValueType::array_type; storage.array_val = new array(val); }
    inline void set_value(const object& val) { type = ValueType::object_type; storage.object_val = new object(val); }
    inline std::string to_string() const {
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
    inline std::string print(bool prettify = false) const { return serialize(prettify ? 0 : -1); }

    inline JsonValue& operator=(const JsonValue& other) {
      if (this != &other) {
        type = other.type;
        storage = other.storage;
      }

      return *this;
    }

    inline bool operator==(const JsonValue& other) const {
      if (type != other.type) {
        return false;
      }

      switch (type) {
        case ValueType::string_type:
          return *(storage.str_val) == *(other.storage.str_val);
        case ValueType::number_type:
          return is_equal(storage.num_val, other.storage.num_val);
        case ValueType::boolean_type:
          return storage.bool_val == other.storage.bool_val;
        case ValueType::object_type: {
          const object* l = this->storage.object_val;
          const object* r = other.storage.object_val;

          return (l->size() == r->size()) && std::equal(l->cbegin(), l->cend(), r->cbegin(),
                  [](const auto& left, const auto& right) {
                    return (left.first == right.first) && (left.second == right.second);
                  });
        }
        case ValueType::array_type: {
          const array* l = this->storage.array_val;
          const array* r = other.storage.array_val;

          return (l->size() == r->size()) && std::equal(l->cbegin(), l->cend(), r->cbegin(),
                  [](const auto& left, const auto& right) {
                    return (left == right);
                  });
        }
        default:
          return true;
      }
    }

    inline bool operator!=(const JsonValue& other) const {
      return !(*this == other);
    }

  private:
    static std::string make_indent(int indent) {
      std::stringstream sstream;
      sstream << '\n';
      for (int i = 0; i < indent * indent_size; i++) {
        sstream << ' ';
      }
      return sstream.str();
    }

    static std::string serialize_str(const std::string& str) {
      return "\"" + str + "\"";
    }

    inline std::string serialize(int indent) const {
      std::stringstream sstream;

      switch (type) {
        case ValueType::string_type:
          sstream << serialize_str(*(storage.str_val));
          break;
        case ValueType::object_type: {
          sstream << '{';
          if (indent != -1) {
            ++indent;
          }
          for (auto citer = storage.object_val->cbegin(); citer != storage.object_val->cend(); ++citer) {
            if (citer != storage.object_val->cbegin()) {
              sstream << ',';
            }
            if (indent != -1) {
              sstream << make_indent(indent);
            }
            sstream << serialize_str(citer->first) << ':';
            if (indent != -1) {
              sstream << ' ';
            }
            sstream << citer->second.serialize(indent);
          }
          if (indent != -1) {
            --indent;
            if (!storage.object_val->empty()) {
              sstream << make_indent(indent);
            }
          }
          sstream << '}';
          break;
        }
        case ValueType::array_type: {
          sstream << '[';
          if (indent != -1) {
            ++indent;
          }
          for (auto citer = storage.array_val->cbegin(); citer != storage.array_val->cend(); ++citer) {
            if (citer != storage.array_val->cbegin()) {
              sstream << ',';
            }
            if (indent != -1) {
              sstream << make_indent(indent);
            }
            sstream << citer->serialize(indent);
          }
          if (indent != -1) {
            --indent;
            if (!storage.array_val->empty()) {
              sstream << make_indent(indent);
            }
          }
          sstream << ']';
          break;
        }
        case ValueType::null_type:
          sstream << "null";
          break;
        case ValueType::number_type: {
          char buf[MAX_NUMBER_STRING_SIZE];
          sstream << std::string(dtoa(buf, storage.num_val));
          break;
        }
        case ValueType::boolean_type:
          sstream << (storage.bool_val ? "true" : "false");
          break;
      }

      return sstream.str();
    }

    Storage storage;
    ValueType type;
  };

  typedef JsonValue::array array;
  typedef JsonValue::object object;
  typedef JsonValue::boolean boolean;
  typedef JsonValue::number number;
  typedef JsonValue::string string;

  bool parse_number(const char** token, double* number);
  std::string parse_string(const char** token);
  bool parse_value(JsonValue& value, const char** token);
  bool parse_object(JsonValue& value, const char** token);
  bool parse_array(JsonValue& value, const char** token);

  bool parse_number(const char** token, double* number) {
    (*token) += strspn((*token), " \t");
    const char* end = (*token) + strcspn((*token), " \t,\n\r}");
    if (end != (*token)) {
      double value;
      if (!atod((*token), end, &value)) return false;
      (*number) = value;
      if (end[0] == TokenType::comma_separator) {
        end++;
      }
      (*token) = end;
      return true;
    }

    return false;
  }

  std::string parse_string(const char** token) {
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

  bool parse_value(JsonValue& value, const char** token) {
    if ((*token)[0] == TokenType::double_quote) {
      // string
      (*token)++;
      std::string str_value = parse_string(token);
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
      if (!parse_number(token, &number)) return false;
      value.set_value(number);
    }

    if ((*token)[0] == TokenType::comma_separator) {
      (*token)++;
    }

    return true;
  }

  bool parse_object(JsonValue& value, const char** token) {
    std::string current_key;
    object root;

    while ((*token)[0]) {
      (*token) += strspn((*token), " \t\n\r");

      if ((*token) == nullptr) return false;

      // end of object
      if ((*token)[0] == TokenType::end_object) {
        (*token)++;
        break;
      }

      // start of key
      if ((*token)[0] == TokenType::double_quote) {
        (*token)++;
        // empty key is not allowed
        if ((*token)[0] == TokenType::double_quote) return false;
        current_key = parse_string(token);
        // no key found
        if (current_key.empty()) return false;
        if ((*token)[0] == TokenType::comma_separator) token++;
        continue;
      }

      if ((*token)[0] == TokenType::start_value) {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");

        JsonValue current_value;
        if ((*token)[0] == TokenType::start_object) {
          (*token)++;
          if (!parse_object(current_value, token)) return false;
        } else if ((*token)[0] == TokenType::start_array) {
          (*token)++;
          if (!parse_array(current_value, token)) return false;
        } else {
          if (!parse_value(current_value, token)) return false;
        }
        root.insert(std::make_pair(current_key, current_value));

        continue;
      }
    }

    if ((*token)[0] == TokenType::comma_separator) {
      (*token)++;
    }

    value.set_value(root);
    return true;
  }

  bool parse_array(JsonValue& value, const char** token) {
    array root;

    while ((*token)[0]) {
      (*token) += strspn((*token), " \t\n\r");

      if ((*token) == nullptr) return false;

      // end of array
      if ((*token)[0] == TokenType::end_array) {
        (*token)++;
        break;
      }

      if ((*token)[0] == TokenType::comma_separator) {
        (*token)++;
      }

      if ((*token)[0] == TokenType::start_object) {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");
        JsonValue current_obj;
        if (!parse_object(current_obj, token)) return false;
        root.emplace_back(current_obj);
        continue;
      } else if ((*token)[0] == TokenType::start_array) {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");
        JsonValue current_arr;
        if (!parse_array(current_arr, token)) return false;
        root.emplace_back(current_arr);
        continue;
      } else {
        (*token) += strspn((*token), " \t\n\r");
        JsonValue current_value;
        if (!parse_value(current_value, token)) return false;
        root.emplace_back(current_value);
        continue;
      }
    }

    if ((*token)[0] == TokenType::comma_separator) {
      (*token)++;
    }

    value.set_value(root);
    return true;
  }

  bool parse(JsonValue& value, const std::string& json) {
    const char* token = json.c_str();
    bool start_of_object = false;
    std::string current_key;
    object root;

    while ((*token)) {
      token += strspn(token, " \t\n\r");

      if (token == nullptr) return false;

      // start of object
      if (token[0] == TokenType::start_object) {
        start_of_object = true;
        token++;
        token += strspn(token, " \t\n\r");
        // empty {} object
        if (token[0] == TokenType::end_object) return true;
        continue;
      }

      // end of json
      if (token[0] == TokenType::end_object) {
        if (!start_of_object) return false;
        start_of_object = !start_of_object;
        token++;
        continue;
      }

      // start of key
      if (token[0] == TokenType::double_quote) {
        token++;
        // empty string is not allowed
        if (token[0] == TokenType::double_quote) return false;
        current_key = parse_string(&token);
        // no key found
        if (current_key.empty()) return false;
        if (token[0] == TokenType::comma_separator) token++;
        continue;
      }

      // start of value
      if (token[0] == TokenType::start_value && start_of_object) {
        token++;
        token += strspn(token, " \t\n\r");

        JsonValue current_value;
        if (token[0] == TokenType::start_object) {
          token++;
          if (!parse_object(current_value, &token)) return false;
        } else if (token[0] == TokenType::start_array) {
          token++;
          if (!parse_array(current_value, &token)) return false;
        } else {
          if (!parse_value(current_value, &token)) return false;
        }
        root.insert(std::make_pair(current_key, current_value));

        continue;
      }
    }

    value.set_value(root);
    return true;
  }
}