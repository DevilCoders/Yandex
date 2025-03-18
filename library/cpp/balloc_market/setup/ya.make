LIBRARY()

NO_UTIL()

OWNER(omakovski)

IF ("${YMAKE}" MATCHES "devtools")
    CFLAGS(-DYMAKE=1)
ENDIF()

SRCS(
    alloc.cpp
    enable.cpp
)

END()
