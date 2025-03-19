PY3_PROGRAM(yc-rkn-config-node)


OWNER(g:cloud-netinfra)


PY_SRCS(
    __init__.py
    alerter.py
    base.py
    bird_configuration.py
    cli.py
    configuration_engine.py
    domain_name_whitelisting.py
    main.py
    network_whitelisting.py
    normalize_content_element_structure.py
    normalize_url_structure.py
    suricata_configuration.py
    whitelists.py
    xml_parser.py
)


PY_MAIN(
    cloud.netinfra.rknfilter.yc_rkn_config_node.main
)


PEERDIR(
    contrib/python/PyYAML
    contrib/python/ConfigArgParse
    contrib/python/netaddr
    contrib/python/dnspython
    cloud/netinfra/rknfilter/yc_rkn_common
    cloud/netinfra/rknfilter/yc_rkn_s3tools
)


END()

