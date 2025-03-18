OWNER(g:mstand)

PY3_LIBRARY()

PEERDIR(
    quality/yaqlib/yaqutils
    quality/yaqlib/clients
)

PY_SRCS(
    NAMESPACE omglib
    __init__.py
    def_values.py
    mc_common.py
    metric_description_struct.py
)

END()
