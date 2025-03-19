PY3_PROGRAM(yc-rkn-filter-node)


OWNER(g:cloud-netinfra)


PY_SRCS(
    __init__.py
    main.py
    cli.py
    demons.py
    bird_configuration.py
    suricata_configuration.py
)


PY_MAIN(
    cloud.netinfra.rknfilter.yc_rkn_filter_node.main
)


PEERDIR(
    contrib/python/PyYAML
    contrib/python/ConfigArgParse
    contrib/python/pytz
    contrib/python/pandas
    cloud/netinfra/rknfilter/yc_rkn_common
    cloud/netinfra/rknfilter/yc_rkn_s3tools
)


END()

