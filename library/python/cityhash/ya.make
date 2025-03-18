PY23_LIBRARY()

OWNER(
    g:yatool
    akastornov
)

SRCS(
    hash.cpp
)

PY_SRCS(
    TOP_LEVEL
    cityhash.pyx
)

END()
