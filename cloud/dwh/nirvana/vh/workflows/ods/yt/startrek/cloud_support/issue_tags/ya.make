OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issue_tags/resources/query.sql ods/yt/startrek/cloud_support/issue_tags/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issue_tags/resources/parameters.yaml ods/yt/startrek/cloud_support/issue_tags/resources/parameters.yaml
)

END()
