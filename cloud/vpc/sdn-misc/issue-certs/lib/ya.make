PY3_LIBRARY()

OWNER(
    sklyaus
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/pem
    contrib/python/cryptography
    contrib/python/requests
    contrib/python/schematics
)

PY_SRCS(
    TOP_LEVEL

    yc_issue_cert/__init__.py
    yc_issue_cert/cluster.py
    yc_issue_cert/config.py
    yc_issue_cert/utils.py
    yc_issue_cert/yc_crt.py
    yc_issue_cert/secret_service.py

    yc_issue_cert/secrets/__init__.py
    yc_issue_cert/secrets/compute_node.py
    yc_issue_cert/secrets/oct.py
    yc_issue_cert/secrets/monops.py
    yc_issue_cert/secrets/vpc_node.py
    yc_issue_cert/secrets/vpc_api.py
    yc_issue_cert/secrets/ylb.py
)

END()
