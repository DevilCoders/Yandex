PY23_LIBRARY()

OWNER(g:yatool)

PY_SRCS(
    prctl.pyx
)

BUILD_ONLY_IF(LINUX)

END()
