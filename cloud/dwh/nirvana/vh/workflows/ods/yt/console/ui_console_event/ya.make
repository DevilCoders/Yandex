OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/console/ui_console_event/resources/query.sql ods/yt/console/ui_console_event/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/console/ui_console_event/resources/parameters.yaml ods/yt/console/ui_console_event/resources/parameters.yaml
)

END()
