PY3_PROGRAM(astest_app_v2)

    OWNER(g:cloud-iam)

    PEERDIR(
        contrib/libs/grpc/src/python/grpcio

        cloud/iam/accessservice/client/iam-access-service-client-python/v1
    )

    PY_MAIN(astest_v2)

    # an alias for the deprecated TOP_LEVEL namespace for PY_SRCS:
    # https://docs.yandex-team.ru/ya-make/manual/python/macros#py_srcs
    PY_NAMESPACE(.)

    PY_SRCS(
        astest_v2.py
    )

END()
