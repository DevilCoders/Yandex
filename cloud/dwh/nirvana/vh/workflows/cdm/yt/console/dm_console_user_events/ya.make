OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/console/dm_console_user_events/resources/parameters.yaml cdm/yt/console/dm_console_user_events/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/cdm/yt/console/dm_console_user_events/resources/query.sql cdm/yt/console/dm_console_user_events/resources/query.sql
)

END()
