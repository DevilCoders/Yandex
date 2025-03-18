PY3_PROGRAM()

OWNER(g:antirobot)

PY_SRCS(__main__.py)

PEERDIR(
    contrib/libs/grpc/src/python/grpcio
    antirobot/captcha_cloud_api/proto
)

END()
