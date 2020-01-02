#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <unordered_map>
#include <list>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include <cassert>

#define USE_UNICODE false

#if defined(_MSC_VER)
#define FORCE_INLINE	__forceinline
#else	// defined(_MSC_VER)
#define	FORCE_INLINE inline __attribute__((always_inline))
#endif

namespace tinyjson {
  const double dbl_epsilon = std::numeric_limits<double>::epsilon();

  constexpr bool is_digit(const char x) {
    return static_cast<unsigned int>((x) - '0') < static_cast<unsigned int>(10);
  }

  FORCE_INLINE bool is_equal(const double a, const double b) {
    return std::fabs(a - b) < dbl_epsilon;
  }

  template <typename K, typename V>
  class linked_hash_map {
  public:
    typedef std::pair<K, V> value_type;
    typedef typename std::list<value_type>::size_type size_type;
    typedef typename std::list<value_type>::iterator iterator;
    typedef typename std::list<value_type>::const_iterator const_iterator;

  public:
    linked_hash_map() : linked_list(), hash_map() {}
    linked_hash_map(size_t size) : linked_list(), hash_map() {
      hash_map.reserve(size);
    }
    linked_hash_map(const linked_hash_map<K, V>& other) : linked_list(), hash_map() {
      for(auto citer = other.cbegin(); citer != other.cend(); citer++) {
        insert(*citer);
      }
    }

    FORCE_INLINE auto insert(const value_type& value) {
      linked_list.emplace_back(value);
      return hash_map.insert(std::make_pair(value.first, std::prev(linked_list.end())));
    }

    FORCE_INLINE bool erase(const K& key) {
      typename std::unordered_map<K, iterator>::iterator iter = hash_map.find(key);
      if (iter == hash_map.end()) {
        return false;
      }
      linked_list.erase(iter->second);
      hash_map.erase(iter);
      return true;
    }

    FORCE_INLINE size_type size() const {
      return linked_list.size();
    }

    FORCE_INLINE bool empty() const {
      return linked_list.empty();
    }

    FORCE_INLINE iterator find(const K& key) {
      typename std::unordered_map<K, iterator>::iterator iter = hash_map.find(key);
      return iter != hash_map.end() ? iter->second : end();
    }

    FORCE_INLINE const_iterator find(const K& key) const {
      typename std::unordered_map<K, iterator>::const_iterator citer = hash_map.find(key);
      return citer != hash_map.cend() ? citer->second : cend();
    }

    FORCE_INLINE iterator begin() { return linked_list.begin(); }
    FORCE_INLINE iterator end() { return linked_list.end(); }
    FORCE_INLINE const_iterator cbegin() const { return linked_list.cbegin(); }
    FORCE_INLINE const_iterator cend() const { return linked_list.cend(); }

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

  FORCE_INLINE bool operator==(const char a, const token_type b) {
    return static_cast<token_type>(a) == b;
  }

  FORCE_INLINE bool operator!=(const char a, const token_type b) {
    return static_cast<token_type>(a) != b;
  }

  // https://stackoverflow.com/questions/2302969/convert-a-float-to-a-string
  static double PRECISION = 0.00000000000001;
  static const int MAX_NUMBER_STRING_SIZE = 32;
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
    friend class json_parser;
  public:
    typedef bool boolean;
    typedef double number;
#if USE_UNICODE
    typedef std::u16string string;
#else
    typedef std::string string;
#endif
    typedef std::vector<json_node*> array;
    typedef linked_hash_map<string, json_node*> object;
    union Storage {
      boolean bool_val;
      number num_val;
      string* str_val;
      array* array_val;
      object* object_val;
    };

    FORCE_INLINE json_node() : storage(), type(node_type::null_type) {}
    FORCE_INLINE json_node(const json_node& other) : storage(), type() { *this = other; }
    explicit json_node(boolean val) : storage(), type(node_type::boolean_type) { storage.bool_val = val; }
    explicit json_node(number val) : storage(), type(node_type::number_type) { storage.num_val = val; }
    explicit json_node(const string& val) : storage(), type(node_type::string_type) { storage.str_val = new string(val); }

#if USE_UNICODE
    explicit json_node(const char16_t* val) : storage(), type(node_type::string_type) { storage.str_val = new string(val); }
#else
    explicit json_node(const char* val) : storage(), type(node_type::string_type) { storage.str_val = new string(val); }
#endif
    explicit json_node(const array& val) : storage(), type() { set(val); }
    explicit json_node(const object& val) : storage(), type() { set(val); }
    ~json_node() {
      clear();
    }

    FORCE_INLINE json_node& get_node(const string& key) {
      static json_node null_node;
      if (!is_object()) return null_node;
      object::iterator iter = storage.object_val->find(key);
      return iter != storage.object_val->end() ? *(iter->second) : null_node;
    }
    FORCE_INLINE const json_node& get_node(const string& key) const {
      static const json_node null_node;
      if (!is_object()) return null_node;
      object::const_iterator citer = storage.object_val->find(key);
      return citer != storage.object_val->cend() ? *(citer->second) : null_node;
    }
    FORCE_INLINE json_node& get_element(const size_t index) {
      static json_node null_node;
      if (!is_array()) return null_node;
      return index < storage.array_val->size() ? *(*storage.array_val)[index] : null_node;
    }
    FORCE_INLINE const json_node& get_element(const size_t index) const {
      static const json_node null_node;
      if (!is_array()) return null_node;
      return index < storage.array_val->size() ? *(*storage.array_val)[index] : null_node;
    }
    FORCE_INLINE boolean get_boolean() const {
      assert(is_boolean());
      return storage.bool_val;
    }
    FORCE_INLINE number get_number() const {
      assert(is_number());
      return storage.num_val;
    }
    FORCE_INLINE string& get_string() const {
      assert(is_string());
      return *(storage.str_val);
    }
    FORCE_INLINE array& get_array() const {
      assert(is_array());
      return *(storage.array_val);
    }
    FORCE_INLINE object& get_object() const {
      assert(is_object());
      return *(storage.object_val);
    }
    FORCE_INLINE json_node& operator[](size_t index) { return this->get_element(index); }
    FORCE_INLINE const json_node& operator[](size_t index) const { return this->get_element(index); }
    FORCE_INLINE json_node& operator[](const string& key) { return this->get_node(key); }
    FORCE_INLINE const json_node& operator[](const string& key) const { return this->get_node(key); }
    FORCE_INLINE bool has(const string& key) const {
      if(!is_object()) return false;
      return storage.object_val->find(key) != storage.object_val->cend();
    }
    FORCE_INLINE size_t length() const {
      switch (type) {
        case node_type::string_type:
          return storage.str_val->size();
        case node_type::array_type:
          return storage.array_val->size();
        case node_type::object_type:
          return storage.object_val->size();
        default:
          return 0;
      }
    }
    FORCE_INLINE std::string to_string() const {
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
    FORCE_INLINE bool to_boolean() const {
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
    FORCE_INLINE string serialize(bool prettify = false, unsigned int indent_size = 2) const {
      string s;
      std::back_insert_iterator<string> iter = std::back_inserter(s);
      _serialize(prettify ? 0 : -1, iter, indent_size);
      return s;
    }
    FORCE_INLINE json_node& operator=(const json_node& other) {
      if (this != &other) {
        clear();

        switch (other.type) {
          case node_type::string_type:
            set(*other.storage.str_val);
            break;
          case node_type::array_type:
            set(*other.storage.array_val);
            break;
          case node_type::object_type:
            set(*other.storage.object_val);
            break;
          default:
            type = other.type;
            storage = other.storage;
            break;
        }
      }

      return *this;
    }
    FORCE_INLINE json_node& operator=(const boolean other) {
      clear();
      set(other);
      return *this;
    }
    FORCE_INLINE json_node& operator=(const double other) {
      clear();
      set(other);
      return *this;
    }
    FORCE_INLINE json_node& operator=(const int other) {
      clear();
      set((number)other);
      return *this;
    }
    FORCE_INLINE json_node& operator=(const string& other) {
      clear();
      set(other);
      return *this;
    }
#if USE_UNICODE
    FORCE_INLINE json_node& operator=(const char16_t* other) {
      clear();
      set(other);
      return *this;
    }
#else
    FORCE_INLINE json_node& operator=(const char* other) {
      clear();
      set(other);
      return *this;
    }
#endif
    FORCE_INLINE json_node& operator=(const array& other) {
      clear();
      set(other);
      return *this;
    }
    FORCE_INLINE json_node& operator=(const object& other) {
      clear();
      set(other);
      return *this;
    }
    FORCE_INLINE bool operator==(const json_node& other) const {
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
    FORCE_INLINE bool operator!=(const json_node& other) const {
      return !(*this == other);
    }
    FORCE_INLINE bool operator==(const string& other) const {
      if (type != node_type::string_type) {
        return false;
      }

      return *(storage.str_val) == other;
    }
    FORCE_INLINE bool operator!=(const string& other) const {
      return !(*this == other);
    }
    FORCE_INLINE bool operator==(const double other) const {
      if (type != node_type::number_type) {
        return false;
      }

      return is_equal(storage.num_val, other);
    }
    FORCE_INLINE bool operator!=(const double other) const {
      return !(*this == other);
    }
    FORCE_INLINE bool operator==(const int other) const {
      if (type != node_type::number_type) {
        return false;
      }

      return is_equal(storage.num_val, (number)other);
    }
    FORCE_INLINE bool operator!=(const int other) const {
      return !(*this == other);
    }
    FORCE_INLINE bool is_null() const { return type == node_type::null_type; }
    FORCE_INLINE bool is_boolean() const { return type == node_type::boolean_type; }
    FORCE_INLINE bool is_number() const { return type == node_type::number_type; }
    FORCE_INLINE bool is_string() const { return type == node_type::string_type; }
    FORCE_INLINE bool is_array() const { return type == node_type::array_type; }
    FORCE_INLINE bool is_object() const { return type == node_type::object_type; }

  private:
    FORCE_INLINE void clear() {
      switch (type) {
        case node_type::string_type:
          delete storage.str_val;
          break;
        case node_type::array_type:
          for (json_node* elem : *(storage.array_val)) {
            delete elem;
          }
          delete storage.array_val;
          break;
        case node_type::object_type:
          for (const auto& elem : *(storage.object_val)) {
            delete elem.second;
          }
          delete storage.object_val;
          break;
        default:
          break;
      }
    }
    FORCE_INLINE void set(boolean val) { type = node_type::boolean_type; storage.bool_val = val; }
    FORCE_INLINE void set(number val) { type = node_type::number_type; storage.num_val = val; }
    FORCE_INLINE void set(const string& val) { type = node_type::string_type; storage.str_val = new string(val); }
#if USE_UNICODE
    FORCE_INLINE void set(const char16_t* val) { type = node_type::string_type; storage.str_val = new string(val); }
#else
    FORCE_INLINE void set(const char* val) { type = node_type::string_type; storage.str_val = new string(val); }
#endif
    FORCE_INLINE void set(const array& val) {
      type = node_type::array_type;
      storage.array_val = new array();
      storage.array_val->reserve(val.size());
      // deep copy
      for (auto e : val) {
        storage.array_val->emplace_back(new json_node(*e));
      }
    }
    FORCE_INLINE void set(const object& val) {
      type = node_type::object_type;
      storage.object_val = new object(val.size());
      // deep copy
      auto begin = val.cbegin();
      auto end = val.cend();
      for (; begin != end; ++begin) {
        storage.object_val->insert(std::make_pair(begin->first, new json_node(*(begin->second))));
      }
    }
    FORCE_INLINE void set(string* val) { type = node_type::string_type; storage.str_val = val; }
    FORCE_INLINE void set(array* val) { type = node_type::array_type; storage.array_val = val; }
    FORCE_INLINE void set(object* val) { type = node_type::object_type; storage.object_val = val; }
    FORCE_INLINE void make_indent(int indent, std::back_insert_iterator<string>& iter, unsigned int indent_size) const {
      size_t size = indent * indent_size;
      iter++ = '\n';
      for (int i = 0; i < size; ++i) {
        iter++ = ' ';
      }
    }
    FORCE_INLINE void serialize_str(const string& str, std::back_insert_iterator<string>& iter) const {
      iter++ = '\"';
      std::copy(str.begin(), str.end(), iter);
      iter++ = '\"';
    }
    void _serialize(int indent, std::back_insert_iterator<string>& iter, unsigned int indent_size) const {
      switch (type) {
        case node_type::string_type:
          serialize_str(*(storage.str_val), iter);
          break;
        case node_type::object_type: {
          iter++ = '{';
          if (indent != -1) {
            ++indent;
          }
          auto cbegin = storage.object_val->cbegin();
          auto cend = storage.object_val->cend();
          for (auto citer = cbegin; citer != cend; ++citer) {
            if (citer != cbegin) {
              iter++ = ',';
            }
            if (indent != -1) {
              make_indent(indent, iter, indent_size);
            }
            serialize_str(citer->first, iter);
            iter++ = ':';
            if (indent != -1) {
              iter++ = ' ';
            }
            citer->second->_serialize(indent, iter, indent_size);
          }
          if (indent != -1) {
            --indent;
            if (!storage.object_val->empty()) {
              make_indent(indent, iter, indent_size);
            }
          }
          iter++ = '}';
          break;
        }
        case node_type::array_type: {
          iter++ = '[';
          if (indent != -1) {
            ++indent;
          }
          auto cbegin = storage.array_val->cbegin();
          auto cend = storage.array_val->cend();
          for (auto citer = cbegin; citer != cend; ++citer) {
            if (citer != cbegin) {
              iter++ = ',';
            }
            if (indent != -1) {
              make_indent(indent, iter, indent_size);
            }
            (*citer)->_serialize(indent, iter, indent_size);
          }
          if (indent != -1) {
            --indent;
            if (!storage.array_val->empty()) {
              make_indent(indent, iter, indent_size);
            }
          }
          iter++ = ']';
          break;
        }
        case node_type::null_type: {
		      static const char* n = "null";
          std::copy(n, n + 4, iter);
          break;
		    }
        case node_type::number_type: {
          char buf[MAX_NUMBER_STRING_SIZE];
          const char* c = dtoa(buf, storage.num_val);
          std::copy(c, c + strlen(c), iter);
          break;
        }
        case node_type::boolean_type: {
		      static const char* t = "true";
          static const char* f = "false";
          if (storage.bool_val) {
            std::copy(t, t + 4, iter);
          } else {
            std::copy(f, f + 5, iter);
          }
          break;
		    }
      }
    }

    Storage storage;
    node_type type;
  };

  typedef json_node::boolean boolean;
  typedef json_node::number number;
  typedef json_node::string string;
  typedef json_node::array array;
  typedef json_node::object object;

  class json_parser {
  public:
    FORCE_INLINE static bool parse(json_node& value, const std::string& json, std::string& err) {
      const char* token = json.c_str();
      err.clear();

      // RFC 4627: only objects or arrays were allowed as root
      if (expect_token(&token, token_type::start_object)) {
        if (!parse_object(value, &token, err)) return false;
      } else if (expect_token(&token, token_type::start_array)) {
        if (!parse_array(value, &token, err)) return false;
      } else {
        return make_err_msg("invalid or empty json.", err);
      }

      return true;
    }

  private:
    FORCE_INLINE static bool expect_token(const char** token, token_type type) {
      (*token) += strspn((*token), " \t\n\r");
      if ((*token)[0] == type) {
        (*token)++;
        return true;
      }

      return false;
    }
    FORCE_INLINE static bool make_err_msg(const char* msg, std::string& err) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%s", msg);
      err = buf;
      return false;
    }
    FORCE_INLINE static bool parse_number(double* number, const char** token) {
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
    FORCE_INLINE static bool parse_string(string& str, const char** token) {
      // skip "
      if ((*token)[0] == token_type::double_quote) (*token)++;
      const char* end = (*token) + strcspn((*token), "\"");
      str.clear();
      if ((*token) != end) {
        str.assign((*token), end - (*token));
        (*token) = ++end;
        return true;
      }

      (*token) = ++end;
      return false;
    }
    FORCE_INLINE static bool parse_value(json_node& value, const char** token, std::string& err) {
      if ((*token)[0] == token_type::double_quote) {
        // string
        string str_value;
        // allow empty string
        parse_string(str_value, token);
        value.set(str_value);
      } else if (((*token)[0] == 't') && (0 == strncmp((*token), "true", 4))) {
        // boolean true
        value.set(true);
        (*token) += 4;
      } else if (((*token)[0] == 'f') && (0 == strncmp((*token), "false", 5))) {
        // boolean false
        value.set(false);
        (*token) += 5;
      } else if (((*token)[0] == 'n') && (0 == strncmp((*token), "null", 4))) {
        // null
        (*token) += 4;
      } else {
        // number
        double number = 0.0;
        if (!parse_number(&number, token)) {
          return make_err_msg("parse error.", err);
        }
        value.set(number);
      }

      return true;
    }
    static bool parse_object(json_node& value, const char** token, std::string& err) {
      string current_key;
      object* root = new object();

      // empty object
      if (expect_token(token, token_type::end_object)) {
        value.set(root);
        return true;
      }

      do {
        if (!expect_token(token, token_type::double_quote)
          || !parse_string(current_key, token)
          || !expect_token(token, token_type::colon)) {
          return make_err_msg("invalid token.", err);
        }

        json_node* current_value = new json_node();
        if (expect_token(token, token_type::start_object)) {
          if (!parse_object(*current_value, token, err)) return false;
        } else if (expect_token(token, token_type::start_array)) {
          if (!parse_array(*current_value, token, err)) return false;
        } else {
          if (!parse_value(*current_value, token, err)) return false;
        }
        root->insert(std::make_pair(current_key, current_value));
      } while(expect_token(token, token_type::comma));

      if (!expect_token(token, token_type::end_object)) {
        return make_err_msg("invalid end of object.", err);
      }

      value.set(root);
      return true;
    }
    static bool parse_array(json_node& value, const char** token, std::string& err) {
      array* root = new array();

      // empty array
      if (expect_token(token, token_type::end_array)) {
        value.set(root);
        return true;
      }

      do {
        json_node* current_value = new json_node();
        if (expect_token(token, token_type::start_object)) {
          if (!parse_object(*current_value, token, err)) return false;
        } else if (expect_token(token, token_type::start_array)) {
          if (!parse_array(*current_value, token, err)) return false;
        } else {
          if (!parse_value(*current_value, token, err)) return false;
        }
        root->emplace_back(current_value);
      } while(expect_token(token, token_type::comma));

      if (!expect_token(token, token_type::end_array)) {
        return make_err_msg("invalid end of array.", err);
      }

      value.set(root);
      return true;
    }
  };
}
