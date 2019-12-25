### tinyjson

Blazing fast header only json parser

### Compiler

compiled and tested on c++14 or upper version.

### One Minute guide

```c++
tinyjson::json_node node1;
string json = R"({"key":"value","obj":{"name":"hello"},"array":[32,99,75]})";
bool result = tinyjson::parse(node1, json);
if (result) {
  // true -> prettify serialize
  // false -> unformatted serialize (default)
  std::cout << node1.serialize(true) << '\n';
}
```

### Tutorial

In RFC 4627, only objects or arrays were allowed as root values of json.
suppose that, node is root of json.

```c++
assert(node.is_object());
assert(node.is_array());
```

Let's query about "key1" is exists or not.

```c++
assert(node.has("key1"));
assert(node["key1"].is_object());
std::cout << node["key1"].serialize(true) << std::endl;
```
```json
{
    "hello": true,
    "hello2": false,
    "hello3": null,
    "hello4": [
      1,
      1.33,
      99.8,
      21.92
    ]
}
```

what about array? below sample code shows how to loop through all the elements.

```c++
tinyjson::json_node& array_node = node["key1"]["hello4"];
assert(array_node.is_array());
for (size_t i = 0; i < array_node.length(); ++i) {
  std::cout << "hello4[" << i << "]: " << array_node[i].get_number() << std::endl;
}
```
```
hello4[0]: 1
hello4[1]: 1.33
hello4[2]: 99.8
hello4[3]: 21.92
```

or, using c++11 range based loop.
note that, array is just a stl vector container. you can use standard iterator as you know.

```c++
tinyjson::json_node& array_node = node["key1"]["hello4"];
assert(array_node.is_array());
tinyjson::array& arr = array_node.get_array();
int i = 0;
for (auto& n : arr) {
  std::cout << "hello4[" << i << "]: " << n.get_number() << std::endl;
  i++;
}
```
```
hello4[0]: 1
hello4[1]: 1.33
hello4[2]: 99.8
hello4[3]: 21.92
```

querying object is same as array.
note that, object is almost same as ordered map. so any kind of standard iterator can be used.

```c++
tinyjson::object& obj_node = node["key1"].get_object();
for (auto& v : obj_node) {
  std::cout << v.second.serialize(true) << std::endl;
}
```
```
true
false
null
[
    1,
    1.33,
    99.8,
    21.92
]
```

comparing values is simple like string compare.

```c++
assert(node == node2);
json_node& first = node.get_node("key1").get_node("hello4").get_element(3);
assert(first == 21.92);
```

there are four kind of operators:

```c++
inline bool operator==(const json_node& other);
inline bool operator==(const string& other);
inline bool operator==(const double other);
inline bool operator==(const int other);
```

operator with json_node compare two json_node as deep equal. the other three operator compare with json_node inner value. 

export to json is a bit complex.

```c++
object root;
root.insert(std::make_pair("key", new json_node("value")));
object inner;
inner.insert(std::make_pair("name", new json_node("hello")));
root.insert(std::make_pair("obj", new json_node(inner)));
auto* arr = new json_node({new json_node(32.0), new json_node(99.0), new json_node(75.0)});
root.insert(std::make_pair("array", arr));
json_node node2(root);
std::cout << node2.serialize(true) << std::endl; // prettify print
```

### Performance benchmark

performance test on macos catalina and clang with large json file which has about 190 MB size:

```
deserialize: 10238.3 ms
serialize: 10451.3 ms
```

another performance test on windows 10 and msvc with same json file:

```
deserialize: 5592.36 ms
serialize: 5552.61 ms
```

### Macro

- USE_UNICODE: determines u16string and u8string.
- INDENT_SIZE: involves serializing output indent size.

### ToDo

- utf8 support
- error message
- json validation