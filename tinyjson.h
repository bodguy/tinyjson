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

    inline auto insert(const value_type& value) {
      linked_list.push_back(value);
      return hash_map.insert(std::make_pair(value.first, std::prev(linked_list.end())));
    }

    inline bool erase(const K& key) {
      typename std::unordered_map<K, iterator>::iterator iter = hash_map.find(key);
      if (iter == hash_map.end()) {
        return false;
      }
      linked_list.erase((*iter).second);
      hash_map.erase(iter);
      return true;
    }

    inline size_type size() const {
      return linked_list.size();
    }

    inline bool empty() const {
      return linked_list.empty();
    }

    inline iterator find(const K& key) {
      auto iter = hash_map.find(key);
      if (iter == hash_map.end()) {
        return end();
      }

      return iter->second;
    }

    inline const_iterator find(const K& key) const {
      auto citer = hash_map.find(key);
      if (citer == hash_map.cend()) {
        return cend();
      }

      return citer->second;
    }

    inline iterator begin() { return linked_list.begin(); }
    inline iterator end() { return linked_list.end(); }
    inline const_iterator cbegin() const { return linked_list.cbegin(); }
    inline const_iterator cend() const { return linked_list.cend(); }

  private:
    std::list<value_type> linked_list;
    std::unordered_map<K, iterator> hash_map;
  };

  enum class node_type {
    null_type = 0,
    boolean_type,
    number_type,
    string_type,
    array_type,
    object_type
  };

  enum class token_type {
    start_object = '{',
    end_object = '}',
    start_array = '[',
    end_array = ']',
    double_quote = '\"',
    colon = ':',
    comma = ','
  };

  inline bool operator==(const char a, const token_type b) {
    return static_cast<token_type>(a) == b;
  }

  inline bool operator!=(const char a, const token_type b) {
    return static_cast<token_type>(a) != b;
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

  class json_node {
  public:
    typedef bool boolean;
    typedef double number;
    typedef std::string string;
    typedef std::vector<json_node> array;
    typedef linked_hash_map<std::string, json_node> object;
    union Storage {
      boolean bool_val;
      number num_val;
      string* str_val;
      array* array_val;
      object* object_val;
    };

    inline json_node() : storage(), type(node_type::null_type) {}
    inline json_node(const json_node& other) : storage(), type(other.type) {
      switch (type) {
        case node_type::string_type:
          storage.str_val = new string(*other.storage.str_val);
          break;
        case node_type::array_type:
          storage.array_val = new array(*other.storage.array_val);
          break;
        case node_type::object_type:
          storage.object_val = new object(*other.storage.object_val);
          break;
        default:
          storage = other.storage;
          break;
      }
    }
    explicit json_node(boolean val) : storage(), type(node_type::boolean_type) { storage.bool_val = val; }
    explicit json_node(number val) : storage(), type(node_type::number_type) { storage.num_val = val; }
    explicit json_node(const string& val) : storage(), type(node_type::string_type) { storage.str_val = new std::string(val); }
    explicit json_node(const char* val) : storage(), type(node_type::string_type) { storage.str_val = new std::string(val); }
    explicit json_node(const array& val) : storage(), type(node_type::array_type) { storage.array_val = new array(val); }
    explicit json_node(const object& val) : storage(), type(node_type::object_type) { storage.object_val = new object(val); }
    inline ~json_node() {
      switch (type) {
        case node_type::string_type:
          delete storage.str_val;
          break;
        case node_type::array_type:
          delete storage.array_val;
          break;
        case node_type::object_type:
          delete storage.object_val;
          break;
        default:
          break;
      }
    }
    inline void set(boolean val) { type = node_type::boolean_type; storage.bool_val = val; }
    inline void set(number val) { type = node_type::number_type; storage.num_val = val; }
    inline void set(const string& val) { type = node_type::string_type; storage.str_val = new std::string(val); }
    inline void set(const char* val) { type = node_type::string_type; storage.str_val = new std::string(val); }
    inline void set(const array& val) { type = node_type::array_type; storage.array_val = new array(val); }
    inline void set(const object& val) { type = node_type::object_type; storage.object_val = new object(val); }
    inline json_node& get_node(const string& key) {
      static json_node null_val;
      if (!is<object>()) return null_val;
      auto iter = storage.object_val->find(key);
      return iter != storage.object_val->end() ? iter->second : null_val;
    }
    inline const json_node& get_node(const string& key) const {
      static json_node null_val;
      if (!is<object>()) return null_val;
      typename object::const_iterator citer = storage.object_val->find(key);
      return citer != storage.object_val->cend() ? citer->second : null_val;
    }
    inline json_node& get_element(const size_t index) {
      static json_node null_val;
      if (!is<array>()) return null_val;
      return index < storage.array_val->size() ? (*storage.array_val)[index] : null_val;
    }
    inline const json_node& get_element(const size_t index) const {
      static json_node null_val;
      if (!is<array>()) return null_val;
      return index < storage.array_val->size() ? (*storage.array_val)[index] : null_val;
    }
    inline bool get(boolean& value) const {
      if (!is<boolean>()) return false;
      value = storage.bool_val;
      return true;
    }
    inline bool get(number& value) const {
      if (!is<number>()) return false;
      value = storage.num_val;
      return true;
    }
    inline bool get(string& value) const {
      if (!is<string>()) return false;
      value = *(storage.str_val);
      return true;
    }
    inline bool get(array& value) const {
      if (!is<array>()) return false;
      value = *(storage.array_val);
      return true;
    }
    inline bool get(object& value) const {
      if (!is<object>()) return false;
      value = *(storage.object_val);
      return true;
    }
    inline bool has(const string& key) const {
      if(!is<object>()) return false;
      return storage.object_val->find(key) != storage.object_val->cend();
    }
    inline std::string to_string() const {
      switch (type) {
        case node_type::string_type:
          return "string";
        case node_type::object_type:
          return "object";
        case node_type::array_type:
          return "array";
        case node_type::null_type:
          return "null";
        case node_type::number_type:
          return "number";
        case node_type::boolean_type:
          return "boolean";
      }
    }
    inline bool to_boolean() const {
      switch (type) {
        case node_type::null_type:
          return false;
        case node_type::boolean_type:
          return storage.bool_val;
        case node_type::number_type:
          return storage.num_val != 0;
        case node_type::string_type:
          return !storage.str_val->empty();
        default:
          return true;
      }
    }
    inline std::string serialize(bool prettify = false) const { return _serialize(prettify ? 0 : -1); }
    inline json_node& operator=(const json_node& other) {
      if (this != &other) {
        type = other.type;

        switch (type) {
          case node_type::string_type:
            storage.str_val = new string(*other.storage.str_val);
            break;
          case node_type::array_type:
            storage.array_val = new array(*other.storage.array_val);
            break;
          case node_type::object_type:
            storage.object_val = new object(*other.storage.object_val);
            break;
          default:
            storage = other.storage;
            break;
        }
      }

      return *this;
    }
    inline bool operator==(const json_node& other) const {
      if (type != other.type) {
        return false;
      }

      switch (type) {
        case node_type::string_type:
          return *(storage.str_val) == *(other.storage.str_val);
        case node_type::number_type:
          return is_equal(storage.num_val, other.storage.num_val);
        case node_type::boolean_type:
          return storage.bool_val == other.storage.bool_val;
        case node_type::object_type: {
          const object* l = this->storage.object_val;
          const object* r = other.storage.object_val;

          return (l->size() == r->size()) && std::equal(l->cbegin(), l->cend(), r->cbegin(),
                  [](const auto& left, const auto& right) {
                    return (left.first == right.first) && (left.second == right.second);
                  });
        }
        case node_type::array_type: {
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
    inline bool operator!=(const json_node& other) const {
      return !(*this == other);
    }
    template <typename T> inline bool is() const { return false; }
    template <> inline bool is<boolean>() const { return type == node_type::boolean_type; }
    template <> inline bool is<number>() const { return type == node_type::number_type; }
    template <> inline bool is<string>() const { return type == node_type::string_type; }
    template <> inline bool is<array>() const { return type == node_type::array_type; }
    template <> inline bool is<object>() const { return type == node_type::object_type; }

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
    inline std::string _serialize(int indent) const {
      std::stringstream sstream;

      switch (type) {
        case node_type::string_type:
          sstream << serialize_str(*(storage.str_val));
          break;
        case node_type::object_type: {
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
            sstream << citer->second._serialize(indent);
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
        case node_type::array_type: {
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
            sstream << citer->_serialize(indent);
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
        case node_type::null_type:
          sstream << "null";
          break;
        case node_type::number_type: {
          char buf[MAX_NUMBER_STRING_SIZE];
          sstream << std::string(dtoa(buf, storage.num_val));
          break;
        }
        case node_type::boolean_type:
          sstream << (storage.bool_val ? "true" : "false");
          break;
      }

      return sstream.str();
    }

    Storage storage;
    node_type type;
  };

  typedef json_node::boolean boolean;
  typedef json_node::number number;
  typedef json_node::string string;
  typedef json_node::array array;
  typedef json_node::object object;

  bool parse_number(const char** token, double* number);
  std::string parse_string(const char** token);
  bool parse_value(json_node& value, const char** token);
  bool parse_object(json_node& value, const char** token);
  bool parse_array(json_node& value, const char** token);

  bool parse_number(const char** token, double* number) {
    (*token) += strspn((*token), " \t");
    const char* end = (*token) + strcspn((*token), " \t,\n\r}]");
    if (end != (*token)) {
      double value;
      if (!atod((*token), end, &value)) return false;
      (*number) = value;
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
      key.assign((*token), offset);
    }

    (*token) = ++end;
    return key;
  }

  bool parse_value(json_node& value, const char** token) {
    if ((*token)[0] == token_type::double_quote) {
      // string
      (*token)++;
      std::string str_value = parse_string(token);
      value.set(str_value);
    } else if (0 == strncmp((*token), "true", 4)) {
      // boolean true
      value.set(true);
      (*token) += 4;
    } else if (0 == strncmp((*token), "false", 5)) {
      // boolean false
      value.set(false);
      (*token) += 5;
    } else if (0 == strncmp((*token), "null", 4)) {
      // null
      (*token) += 4;
    } else {
      // number
      double number = 0.0;
      if (!parse_number(token, &number)) return false;
      value.set(number);
    }

    return true;
  }

  bool parse_object(json_node& value, const char** token) {
    std::string current_key;
    object root;

    while ((*token)[0]) {
      (*token) += strspn((*token), " \t\n\r");

      if ((*token) == nullptr) return false;

      if ((*token)[0] == token_type::comma) {
        (*token)++;
      }

      // end of object
      if ((*token)[0] == token_type::end_object) {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");
        break;
      }

      // start of key
      if ((*token)[0] == token_type::double_quote) {
        (*token)++;
        current_key = parse_string(token);
        // empty key is not allowed
        if (current_key.empty()) return false;
        continue;
      }

      if ((*token)[0] == token_type::colon) {
        (*token)++;
        (*token) += strspn((*token), " \t\n\r");

        json_node current_value;
        if ((*token)[0] == token_type::start_object) {
          (*token)++;
          if (!parse_object(current_value, token)) return false;
        } else if ((*token)[0] == token_type::start_array) {
          (*token)++;
          if (!parse_array(current_value, token)) return false;
        } else {
          if (!parse_value(current_value, token)) return false;
        }
        root.insert(std::make_pair(current_key, current_value));

        continue;
      }
    }

    value.set(root);
    return true;
  }

  bool parse_array(json_node& value, const char** token) {
    array root;

    while ((*token)[0]) {
      (*token) += strspn((*token), " \t\n\r");

      if ((*token) == nullptr) return false;

      // end of array
      if ((*token)[0] == token_type::end_array) {
        (*token)++;
        break;
      }

      if ((*token)[0] == token_type::comma) {
        (*token)++;
        continue;
      }

      if ((*token)[0] == token_type::start_object) {
        (*token)++;
        json_node current_obj;
        if (!parse_object(current_obj, token)) return false;
        root.emplace_back(current_obj);
        continue;
      } else if ((*token)[0] == token_type::start_array) {
        (*token)++;
        json_node current_arr;
        if (!parse_array(current_arr, token)) return false;
        root.emplace_back(current_arr);
        continue;
      } else {
        json_node current_value;
        if (!parse_value(current_value, token)) return false;
        root.emplace_back(current_value);
        continue;
      }
    }

    value.set(root);
    return true;
  }

  bool deserialize(json_node& value, const std::string& json) {
    const char* token = json.c_str();
    bool start_of_object = false;
    std::string current_key;
    object root;

    while ((*token)) {
      token += strspn(token, " \t\n\r");

      if (token == nullptr) return false;

      if (token[0] == token_type::comma) {
        token++;
      }

      // start of object
      if (token[0] == token_type::start_object) {
        start_of_object = true;
        token++;
        token += strspn(token, " \t\n\r");
        // empty {} object
        if (token[0] == token_type::end_object) return true;
        continue;
      }

      // end of json
      if (token[0] == token_type::end_object) {
        if (!start_of_object) return false;
        start_of_object = !start_of_object;
        token++;
        continue;
      }

      // start of key
      if (token[0] == token_type::double_quote) {
        token++;
        current_key = parse_string(&token);
        // empty string is not allowed
        if (current_key.empty()) return false;
        continue;
      }

      // start of value
      if (token[0] == token_type::colon && start_of_object) {
        token++;
        token += strspn(token, " \t\n\r");

        json_node current_value;
        if (token[0] == token_type::start_object) {
          token++;
          if (!parse_object(current_value, &token)) return false;
        } else if (token[0] == token_type::start_array) {
          token++;
          if (!parse_array(current_value, &token)) return false;
        } else {
          if (!parse_value(current_value, &token)) return false;
        }
        root.insert(std::make_pair(current_key, current_value));

        continue;
      }
    }

    value.set(root);
    return true;
  }
}