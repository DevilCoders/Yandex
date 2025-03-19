OWNER(g:cloud-infra)

PY3_PROGRAM()

PEERDIR(
    cloud/infra/packages/lib
    contrib/python/pyaml
)

PY_SRCS(
    configure_network.py
    gen_trottling_info.py
    gen_yc_infra_inspector_config.py
    nvme_part_label.py
    nvme_split.py
)

NO_CHECK_IMPORTS()

END()
