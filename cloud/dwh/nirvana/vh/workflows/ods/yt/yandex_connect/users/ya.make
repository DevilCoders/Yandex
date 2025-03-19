OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/yandex_connect/users/resources/query.sql ods/yt/yandex_connect/users/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/yandex_connect/users/resources/parameters.yaml ods/yt/yandex_connect/users/resources/parameters.yaml
)

END()
