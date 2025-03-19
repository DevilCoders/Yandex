PY2_LIBRARY()

OWNER(
    g:turbo
    zhenyok
)

PEERDIR(
    kernel/turbo/canonizer
)

PY_SRCS( NAMESPACE turbo
    canonizer.swg
    __init__.py
)

END()
