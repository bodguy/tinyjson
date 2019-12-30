### tinyjson

Blazing fast header only json parser

### Compiler

compiled and tested on c++14 or higher version.

### One Minute guide

```c++
tinyjson::json_node node;
std::string err;
string json = R"({"key":"value","obj":{"name":"hello"},"array":[32,99,75]})";
bool result = tinyjson::json_parser::parse(node, json, err);
if (!result) {
  std::cout << err << std::endl;
  return -1;
}
// true -> prettify serialize
// false -> unformatted serialize (default)
std::cout << node.serialize(true) << std::endl;
```
```json
{
  "key": "value",
  "obj": {
    "name": "hello"
  },
  "array": [
    32,
    99,
    75
  ]
}
```

### Tutorial

In [RFC 4627](https://tools.ietf.org/html/rfc4627), only objects or arrays were allowed as root values of json.

```c++
assert(node.is_object());
assert(node.is_array());
```

### Get the values

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

[] based approach is convenient like accessing plain javascript object.
below code shows function based approach equivalent which is a bit messy.

```c++
std::cout << node.get_node("key1").serialize(true) << std::endl;
``` 

### Set the values

setting values are simple as getting values. just take a value from [] operator and assign other values to this.

```c++
object root;
root.insert(std::make_pair("key", new json_node("value")));
node["key1"]["hello4"] = root;
std::cout << node.serialize(true) << std::endl;
```
```json
{
    "hello": true,
    "hello2": false,
    "hello3": null,
    "hello4": {
      "key": "value"
    }
}
```

as you know, assigning is also work with number, array, object, boolean and string.

```c++
node["key1"]["hello4"] = "hello world";
node["key1"]["hello3"] = 10.22;
std::cout << node.serialize(true) << std::endl;
```
```json
{
    "hello": true,
    "hello2": false,
    "hello3": 10.22,
    "hello4": "hello world"
}
```

but beware that, assigning null json value should be as follows.

```c++
node["key1"]["hello"] = json_node();
```
```
"hello": null
```

beacuse empty json_node is same as null json value.

you can assign a json_node directly.

```c++
object root;
root.insert(std::make_pair("key", new json_node("value")));
object inner;
inner.insert(std::make_pair("name", new json_node("hello")));
root.insert(std::make_pair("obj", new json_node(inner)));
auto* arr = new json_node({new json_node(32.0), new json_node(99.0), new json_node(75.0)});
root.insert(std::make_pair("array", arr));
json_node node2(root);
node["key1"]["hello4"] = node2;
```
```json
{
  "key1": {
    "hello": true,
    "hello2": false,
    "hello3": null,
    "hello4": {
      "key": "value",
      "obj": {
        "name": "hello"
      },
      "array": [
        32,
        99,
        75
      ]
    }
  }
}
```

there are another case assigning same json_node instance. this will cause a undefined behaviour like memory leak.

### Query array

what about array? below sample code shows how to loop through all the elements.  
but beware that, json_node.length() function works with string, array and object as well.  
so make sure json_node is array type before looping array elements.

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
for (auto n : arr) {
  std::cout << "hello4[" << i << "]: " << n->get_number() << std::endl;
  i++;
}
```
```
hello4[0]: 1
hello4[1]: 1.33
hello4[2]: 99.8
hello4[3]: 21.92
```

### Query object

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

### Compare values

comparing values are simple like string compare.

```c++
assert(node == node2);
json_node& first = node.get_node("key1").get_node("hello4").get_element(3);
// OR node["key1"]["hello4"][3];
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

### Export to json

export to json is a bit complex now.

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
```json
{
  "key": "value",
  "obj": {
    "name": "hello"
  },
  "array": [
    32,
    99,
    75
  ]
}
```

### Number

The number is represented by e-notation.

```json
{
  "e-not": 3.125e7
}
```

above example can be expressed as following.

```json
{
  "e-not": 31250000
}
```

### Performance benchmark

tested on MackBook Pro 2.5Ghz Quad core i7, 16GB RAM  
with json file which has about 190 MB size.

clang 9.0.0:

```
deserialize: 10238.3 ms
serialize: 10451.3 ms
```

msvc 16:

```
deserialize: 5592.36 ms
serialize: 5552.61 ms
```

### Macro

- USE_UNICODE: determines which one use from u16string and u8string.

### ToDo

- utf8 support: switching to utf16string