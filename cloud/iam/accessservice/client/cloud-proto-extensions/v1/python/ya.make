OWNER(g:cloud-iam)

PY3_LIBRARY(yc_proto_extensions)

# an alias for the deprecated TOP_LEVEL namespace for PY_SRCS:
# https://docs.yandex-team.ru/ya-make/manual/python/macros#py_srcs
PY_NAMESPACE(.)

PY_SRCS(
    yandex/cloud/protoextensions/__init__.py
    yandex/cloud/protoextensions/sensitive_pb2.py
)

END()
