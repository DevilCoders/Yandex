PY2TEST()

OWNER(g:antiadblock)

PEERDIR(
    antiadblock/libs/tornado_redis
)

PY_DOCTESTS(
    antiadblock.libs.tornado_redis.lib.dc
)

END()
