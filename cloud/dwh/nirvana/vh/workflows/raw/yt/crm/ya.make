OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
    cloud/dwh/nirvana/vh/common
)

PY_SRCS(__init__.py)

END()
