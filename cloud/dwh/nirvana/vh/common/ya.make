OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    library/python/resource
    library/python/reactor/client

    nirvana/valhalla/src
    contrib/python/requests
    contrib/python/pandas
    contrib/python/PyYAML

    cloud/dwh/utils
)

PY_SRCS(
    workflows/__init__.py
    workflows/base.py
    workflows/lag_monitor.py
    workflows/python_base.py
    workflows/solomon_to_yt.py
    workflows/solomon_cpu_metrics.py
    workflows/yql.py
    workflows/yt_to_clickhouse.py

    __init__.py
    operations.py
    chyt.py
    utils.py
    yt.py
)

RESOURCE(
    cloud/dwh/yql/utils/datetime.sql yql/utils/datetime.sql
    cloud/dwh/yql/utils/helpers.sql yql/utils/helpers.sql
    cloud/dwh/yql/utils/numbers.sql yql/utils/numbers.sql
    cloud/dwh/yql/utils/tables.sql yql/utils/tables.sql
    cloud/dwh/nirvana/vh/common/resources/generate_drop_tables_query.py common/resources/generate_drop_tables_query.py
    cloud/dwh/nirvana/vh/common/resources/generate_solomon_metric_for_lag_monitoring.py common/resources/generate_solomon_metric_for_lag_monitoring.py
    cloud/dwh/yql/utils/currency.sql yql/utils/currency.sql
)

END()
