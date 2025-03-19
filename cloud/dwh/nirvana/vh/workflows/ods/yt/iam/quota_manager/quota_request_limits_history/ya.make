OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/quota_manager/quota_request_limits_history/resources/query.sql ods/yt/iam/quota_manager/quota_request_limits_history/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/quota_manager/quota_request_limits_history/resources/parameters.yaml ods/yt/iam/quota_manager/quota_request_limits_history/resources/parameters.yaml
)

END()
