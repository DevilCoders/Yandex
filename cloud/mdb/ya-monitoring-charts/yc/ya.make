OWNER(munakoiso)

PY2_PROGRAM(v3api_dashboards)

PY_SRCS(
    __main__.py
)

PEERDIR(
    solomon/protos/monitoring/v3
    solomon/protos/monitoring/v3/cloud
    solomon/protos/monitoring/v3/cloud/priv
)

NO_CHECK_IMPORTS()

END()

