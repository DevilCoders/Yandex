PY23_LIBRARY()

OWNER(
    m-milkin
    g:base
    g:neural-search
)

PEERDIR(
    kernel/dssm_applier/utils
)

PY_SRCS(
    encode.pyx
)

END()
