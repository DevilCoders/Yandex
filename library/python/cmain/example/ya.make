PY23_LIBRARY()

OWNER(borman)

PEERDIR(
    library/python/cmain
)

PY_SRCS(main.pyx)

SRCS(
    main.cpp
)

END()

RECURSE(
    py2
    py3
)
