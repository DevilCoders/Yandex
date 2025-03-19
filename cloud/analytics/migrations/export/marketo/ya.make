PY3_PROGRAM(migrate)

OWNER(g:cloud_analytics)

PEERDIR(
    cloud/analytics/scripts/yt_migrate

    yt/python/client
)

PY_MAIN(cloud.analytics.scripts.yt_migrate)

PY_SRCS(
    0001_add_total_amount_column.py
    0002_add_custom_columns.py
)

RESOURCE(

)

END()
