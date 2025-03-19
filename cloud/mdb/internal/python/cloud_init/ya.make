PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/PyYAML
)

PY_SRCS(
    __init__.py
    config/__init__.py
    config/base.py
    config/write_files.py
    config/config.py
    config/manage_etc_hosts.py
    config/legacy_key_value.py
    config/users.py
    config/runcmd.py
    config/hostname.py
    yaml/__init__.py
    yaml/modules.py
    yaml/cloud_config.py
)

END()

RECURSE_FOR_TESTS(
    test
)
