OWNER(g:cloud_analytics)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
    cloud/dwh/nirvana/config

    cloud/analytics/nirvana/vh/config
    cloud/analytics/nirvana/vh/operations
    cloud/analytics/nirvana/vh/types
    cloud/analytics/nirvana/vh/utils

    cloud/analytics/nirvana/vh/workflows/dictionaries/ids/ba_cloud_folder_dict
)

PY_SRCS(__init__.py)

END()
