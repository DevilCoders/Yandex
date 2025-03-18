PY23_LIBRARY()

OWNER(
    torkve
    max7255
)

IF(OS_LINUX)
PY_SRCS(
    __init__.py
    nstools.pyx
)
ELSE()
PY_SRCS(
    nstools.py
)
ENDIF()

END()
