PY3_PROGRAM(migrate)

OWNER(g:cloud_analytics)

PEERDIR(
    cloud/analytics/scripts/yt_migrate
    yql/library/python

    yt/python/client_with_rpc
)

PY_MAIN(cloud.analytics.scripts.yt_migrate)

PY_SRCS(
    0000_initial.py
    0001_add_marketo_fields.py
    0002_make_dwh_id_only_key.py
    0003_passport_uid_index.py
    0004_email_index.py
    0005_add_forms_fields.py
    0006_lead_id.py
    0007_lead_id_index.py
    0008_added_ba_fields.py
    0009_add_person_type.py
    0010_add_isv_var_fields.py
)

RESOURCE(

)

END()
