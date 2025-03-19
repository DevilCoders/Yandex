OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    contrib/python/aiohttp

    cloud/dwh/utils
)

PY_SRCS(__init__.py)

END()
