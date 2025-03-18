PY23_LIBRARY()

OWNER(g:passport_python)

PEERDIR(
    contrib/python/six
    library/python/cookiemy/srcs
)

PY_SRCS(
    TOP_LEVEL
    cookiemy.pyx
)

END()

# эта библиотека никогда не собиралась под win
IF (NOT OS_WINDOWS)
    RECURSE(
        so
    )
ENDIF()
