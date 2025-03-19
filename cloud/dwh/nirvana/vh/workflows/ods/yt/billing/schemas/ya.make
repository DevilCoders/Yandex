OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/schemas/resources/query.sql ods/yt/billing/schemas/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/schemas/resources/parameters.yaml ods/yt/billing/schemas/resources/parameters.yaml
)

END()
