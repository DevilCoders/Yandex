PY2_LIBRARY()

OWNER(torkve)

IF(OS_LINUX)
PY_SRCS(
    __init__.py
    sendmsg.pyx
)

# musl code for CMSG_NXTHDR is broken by this check
CFLAGS(-Wno-sign-compare)

ELSE()
PY_SRCS(
    __init__.py
    sendmsg.py
)
ENDIF()

END()

RECURSE_FOR_TESTS(tests)
