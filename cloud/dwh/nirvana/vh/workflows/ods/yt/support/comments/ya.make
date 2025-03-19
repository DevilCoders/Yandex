OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/support/comments/resources/query.sql ods/yt/support/comments/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/support/comments/resources/parameters.yaml ods/yt/support/comments/resources/parameters.yaml
)

END()
