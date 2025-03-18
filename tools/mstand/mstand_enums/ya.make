OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    quality/yaqlib/yaqlibenums
)

PY_SRCS(
    NAMESPACE mstand_enums
    __init__.py
    mstand_general_enums.py
    mstand_offline_enums.py
    mstand_online_enums.py
)

END()
