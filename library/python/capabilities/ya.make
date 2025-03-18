OWNER(torkve)

PY23_LIBRARY()

IF(OS_LINUX)
PY_SRCS(
    __init__.py
    capabilities.pyx
)
PEERDIR(contrib/libs/libcap)
ELSE()
PY_SRCS(
    __init__.py
    capabilities.py
)
ENDIF()

END()

RECURSE_FOR_TESTS(tests)
