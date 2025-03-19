OWNER(
    g:ugc
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    generate_identifier.pyx
)

PEERDIR(
    kernel/ugc/security/lib
)

END()
