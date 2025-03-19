PY3_PROGRAM(yc-rkn-begger-node)


OWNER(g:cloud-netinfra)


PY_SRCS(
    __init__.py
    cli.py
    local_files.py
    main.py
    rkn_api_communication.py
    rkn_interaction.py
    rkn_request_file.py
)


PY_MAIN(
    cloud.netinfra.rknfilter.yc_rkn_begger_node.main
)


PEERDIR(
    contrib/python/PyYAML
    contrib/python/ConfigArgParse
    contrib/python/pytz
    contrib/python/pandas
    contrib/python/suds-jurko
    cloud/netinfra/rknfilter/yc_rkn_common
    cloud/netinfra/rknfilter/yc_rkn_s3tools
)


END()

