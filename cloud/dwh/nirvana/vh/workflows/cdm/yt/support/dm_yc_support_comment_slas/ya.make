OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_comment_slas/resources/query.sql cdm/yt/support/dm_yc_support_comment_slas/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_comment_slas/resources/parameters.yaml cdm/yt/support/dm_yc_support_comment_slas/resources/parameters.yaml
)

END()
