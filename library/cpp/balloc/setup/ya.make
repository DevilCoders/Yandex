LIBRARY()

NO_UTIL()

OWNER(ironpeter)

IF ("${YMAKE}" MATCHES "devtools")
    CFLAGS(-DYMAKE=1)
ENDIF()

SRCS(
    alloc.cpp
    enable.cpp
)

END()

NEED_CHECK()
