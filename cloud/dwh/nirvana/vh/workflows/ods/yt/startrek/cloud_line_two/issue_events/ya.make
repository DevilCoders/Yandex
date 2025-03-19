OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issue_events/resources/query.sql ods/yt/startrek/cloud_line_two/issue_events/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issue_events/resources/parameters.yaml ods/yt/startrek/cloud_line_two/issue_events/resources/parameters.yaml
)

END()
