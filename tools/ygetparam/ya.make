OWNER(
    g:antiinfra
)

PY2_LIBRARY()

PY_SRCS(
    ygetparam.py
    curly_braces_expander.py
    ygetparam_data_loader.py
)

PEERDIR(
    contrib/python/urllib3
    tools/ygetparam/ygetparam_modules
    tools/ygetparam/ygetparam_modules/etcd
)

END()
