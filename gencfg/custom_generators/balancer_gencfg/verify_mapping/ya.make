OWNER(
    g:balancer
    mvel
)

PY2_PROGRAM()

PY_SRCS(
    __main__.py
)

PEERDIR(
    gencfg/custom_generators/balancer_gencfg
    contrib/python/coloredlogs
    contrib/python/requests
    contrib/python/retry
    search/tools/devops/libs
)

END()
