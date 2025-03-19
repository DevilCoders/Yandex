#!/bin/bash

python3 -m grpc_tools.protoc -I ../../../normalizer/proto/ --python_out=. --grpc_python_out=. ../../../normalizer/proto/normalizer.proto

sed -i.bak -E 's/import normalizer_pb2/from . import normalizer_pb2/g' normalizer_pb2_grpc.py
rm -f normalizer_pb2_grpc.py.bak
