Argparse protobuf library
====
Library for parsing program arguments to protobuf message.
This library for python is written inspired by [this library](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/getoptpb) described in [this post](https://clubs.at.yandex-team.ru/arcadia/15089).
The main idea is to describe arguments of your program and their types as protobuf message and generate command line arguments based on this message.
As an additional you can pass file with config in yaml or json or protbuf text format and message will be filled in from it.

Here is [example](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/protobuf/argparse/example), also you can see how it can be used in [unit tests](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/protobuf/argparse/ut)
The smallest example is to write proto file
Example:
```protobuf
message TMyArgs {
    required string MyFirstOption = 1;
};
```
add it to ya.make and in program write
```python
import path.to.your.program.proto_file_pb2 as arguments_proto
from library.python.protobuf.argparse import ArgumentParser

def main():
    parser = ArgumentParser(arguments_proto.TMyArgs)
    args, my_args = parser.parse_args()
```
Here **my_args** is protobuf object of class TMyArgs and args is usual arguments given by argparse.ArgumentParser.
You can also use any features of **argparse.ArgumentParser**, because **library.python.protobuf.argparse.ArgumentParser** is it's extension.
