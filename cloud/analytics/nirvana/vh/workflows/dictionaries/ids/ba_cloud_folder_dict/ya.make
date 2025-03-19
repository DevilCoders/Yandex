OWNER(g:cloud_analytics)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/analytics/nirvana/vh/workflows/dictionaries/ids/ba_cloud_folder_dict/resources/query.sql dictionaries/ids/ba_cloud_folder_dict/resources/query.sql
    cloud/analytics/nirvana/vh/workflows/dictionaries/ids/ba_cloud_folder_dict/resources/parameters.json dictionaries/ids/ba_cloud_folder_dict/resources/parameters.json

    cloud/dwh/yql/utils/datetime.sql yql/utils/datetime.sql
    cloud/dwh/yql/utils/tables.sql yql/utils/tables.sql
)

END()
