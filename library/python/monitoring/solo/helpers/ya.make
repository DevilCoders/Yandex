OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    awacs/__init__.py
    awacs/alerts.py
    awacs/dashboards.py
    logbroker/__init__.py
    logbroker/alerts.py
    logbroker/dashboards.py
    logbroker/sensors.py
    logfeller/__init__.py
    logfeller/alerts.py
    logfeller/sensors.py
    solomon/__init__.py
    solomon/alerts.py
    yasm/__init__.py
    yasm/alerts.py
    yasm/signals.py
    yt/__init__.py
    yt/alerts.py
    yt/sensors.py
    cli.py
    __init__.py
    juggler/__init__.py
    juggler/dashboard.py
)

PEERDIR(
    library/python/monitoring/solo/controller
    library/python/monitoring/solo/util
    library/python/monitoring/solo/objects/solomon/sensor
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/objects/solomon/v3
    library/python/monitoring/solo/objects/juggler

    library/python/oauth
    library/python/vault_client

    contrib/python/juggler_sdk
)

END()

