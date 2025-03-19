OWNER(g:cloud-iam)

PY3_LIBRARY(yc_as_api_proto_v2)

PEERDIR(
    contrib/libs/grpc/src/python/grpcio
    contrib/python/protobuf

    cloud/iam/accessservice/client/cloud-proto-extensions/v1/python
)

# an alias for the deprecated TOP_LEVEL namespace for PY_SRCS:
# https://docs.yandex-team.ru/ya-make/manual/python/macros#py_srcs
PY_NAMESPACE(.)

PY_SRCS(
    yandex/cloud/priv/accessservice/v2/__init__.py
    yandex/cloud/priv/accessservice/v2/access_service_pb2.py
    yandex/cloud/priv/accessservice/v2/access_service_pb2_grpc.py
    yandex/cloud/priv/accessservice/v2/resource_pb2.py
    yandex/cloud/priv/accessservice/v2/resource_pb2_grpc.py
)

END()
