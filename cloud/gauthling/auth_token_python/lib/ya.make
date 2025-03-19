PY23_LIBRARY(yc_auth_token)

OWNER(g:cloud)

PEERDIR(cloud/gauthling/auth_token_protobuf)

PY_SRCS(
    NAMESPACE yc_auth_token
    __init__.py
    utils.py
)

END()
