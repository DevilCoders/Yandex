PY23_LIBRARY()

OWNER(g:passport_python)

ADDINCL(
    contrib/libs/lang_detect/include
)

PEERDIR(
    contrib/libs/lang_detect
)

PY_SRCS(
    TOP_LEVEL
    langdetect.pyx
)

END()
