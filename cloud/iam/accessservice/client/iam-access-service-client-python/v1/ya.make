OWNER(g:cloud-iam)

PY3_LIBRARY(yc_as_client_lib_v2)

    PEERDIR(
        contrib/libs/grpc/src/python/grpcio

        cloud/iam/accessservice/client/iam-access-service-api-proto/v1/python
    )

    # an alias for the deprecated TOP_LEVEL namespace for PY_SRCS:
    # https://docs.yandex-team.ru/ya-make/manual/python/macros#py_srcs
    PY_NAMESPACE(.)

    ALL_PY_SRCS(RECURSIVE yc_as_client_v2)

END()

RECURSE_FOR_TESTS(
    ut
)
