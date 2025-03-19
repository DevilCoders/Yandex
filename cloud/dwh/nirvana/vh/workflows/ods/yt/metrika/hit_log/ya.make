OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/metrika/hit_log/resources/query.sql ods/yt/metrika/hit_log/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/metrika/hit_log/resources/parameters.yaml ods/yt/metrika/hit_log/resources/parameters.yaml
)

END()
