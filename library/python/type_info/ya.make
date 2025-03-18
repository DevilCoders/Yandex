PY23_LIBRARY()

OWNER(
    g:yt
)

PY_SRCS(
    NAMESPACE yandex.type_info

    __init__.py
    typing.py
    extension.py
    bindings.pyx
)

PEERDIR(
    library/cpp/type_info
    contrib/python/six
)

END()
