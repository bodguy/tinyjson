### tinyjson

Blazing fast header only json parser

### How to use

sample json file.
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

simple deserialize & serialize example.
```c++
json_node node1;
string json = R"({"key":"value","obj":{"name":"hello"},"array":[32,99,75]})";
bool result = tinyjson::parse(node1, json);
if (result) {
std::cout << node1.serialize(true) << '\n'; // prettify print
}
```

export to json example.
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

check both json are equals.
```c++
std::cout << std::boolalpha << (node1 == node2) << std::endl; // true
```

procedural json node acquire.
```c++
if (node1.is_object()) {
  json_node* val = node1.get_node("array")->get_element(1);
  std::cout << val->serialize() << std::endl; // 99
}
```

procedural json node acquire 2.
```c++
// get specific string value
json_node* key1_node = node2.get_node("key1");
string key1_value;
if (key1_node->get(key1_value)) {
 std::cout << key1_value << std::endl; // value
}

// loop from root of json object
object root;
if (node2.get(root)) {
 for (auto& v : root) {
   std::cout << v.second->serialize(true) << std::endl;
 }
}
/*
 "value"
 {
   "name": "min123"
 }
 [
   32,
   75,
   99
 ]
*/
```

### Performance

performance test on macos catalina and clang with large json file which has about 190 MB size:

```
deserialize: 10562.8 ms
serialize: 10841.7 ms
```

another performance test on windows 10 and msvc with same json file:

```
deserialize: 5592.36 ms
serialize: 5552.61 ms
```

### ToDo

- utf8 support
- json validation
- more convenient api
