PY23_LIBRARY()

OWNER(iakolzin)

PEERDIR(
    library/cpp/on_disk/aho_corasick
)

PY_SRCS(
    __init__.py
    __aho_corasick.pyx
)

END()
