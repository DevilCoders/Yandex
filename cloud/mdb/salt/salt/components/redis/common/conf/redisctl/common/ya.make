OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE common
    __init__.py
    functions.py
)

PEERDIR(
    cloud/mdb/salt/salt/components/redis/common/conf/redisctl/utils
)

END()
