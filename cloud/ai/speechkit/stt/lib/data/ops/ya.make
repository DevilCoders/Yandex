PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    collect_records/__init__.py
    collect_records/collect_records.py
    collect_records/proxy_logs.py
    collect_records/record_creator.py
    queries/__init__.py
    queries/common_records_tags.py
    queries/records.py
    queries/records_bits.py
    queries/records_bits_markups.py
    queries/records_joins.py
    queries/records_tags.py
    queries/select.py
    yt/__init__.py
    yt/common.py
    yt/markups_assignments.py
    yt/markups_params.py
    yt/markups_pools.py
    yt/metrics.py
    yt/recognitions.py
    yt/records.py
    yt/records_bits.py
    yt/records_bits_markups.py
    yt/records_joins.py
    yt/records_tags.py
    __init__.py
)

PEERDIR(
    cloud/ai/lib/python/serialization
    cloud/ai/lib/python/datasource/yt/ops
    cloud/ai/speechkit/stt/lib/data/model
    yt/python/client
    yql/library/python
)

END()
