PY3_PROGRAM(migrate)

OWNER(g:cloud_analytics)

PEERDIR(
    cloud/analytics/scripts/yt_migrate
    yql/library/python

    yt/python/client
)

PY_MAIN(cloud.analytics.scripts.yt_migrate)

PY_SRCS(
    0001_add_lead_dwh_id_column.py
)

RESOURCE(

)

END()
