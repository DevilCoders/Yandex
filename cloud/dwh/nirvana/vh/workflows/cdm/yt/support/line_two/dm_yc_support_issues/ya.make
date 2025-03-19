OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_yc_support_issues/resources/query.sql cdm/yt/support/line_two/dm_yc_support_issues/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_yc_support_issues/resources/parameters.yaml cdm/yt/support/line_two/dm_yc_support_issues/resources/parameters.yaml
)

END()
