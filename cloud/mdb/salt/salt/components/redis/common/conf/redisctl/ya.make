OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE redisctl
    __init__.py
    redisctl.py
)

PEERDIR(
    cloud/mdb/salt/salt/components/redis/common/conf/redisctl/commands
    cloud/mdb/salt/salt/components/redis/common/conf/redisctl/utils
)

END()

RECURSE(
    commands
    utils
)

RECURSE_ROOT_RELATIVE(
    cloud/mdb/salt-tests/components/redis
)
