OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/quota_limits/resources/query.sql ods/yt/compute/quota_limits/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/quota_limits/resources/parameters.yaml ods/yt/compute/quota_limits/resources/parameters.yaml
)

END()
