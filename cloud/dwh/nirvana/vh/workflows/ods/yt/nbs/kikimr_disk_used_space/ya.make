OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/nbs/kikimr_disk_used_space/resources/query.sql ods/yt/nbs/kikimr_disk_used_space/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/nbs/kikimr_disk_used_space/resources/parameters.yaml ods/yt/nbs/kikimr_disk_used_space/resources/parameters.yaml

    cloud/dwh/yql/utils/tables.sql yql/utils/tables.sql
)

END()
