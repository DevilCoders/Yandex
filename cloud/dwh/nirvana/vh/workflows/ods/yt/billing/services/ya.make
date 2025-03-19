OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/services/resources/query.sql ods/yt/billing/services/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/services/resources/parameters.yaml ods/yt/billing/services/resources/parameters.yaml
)

END()
