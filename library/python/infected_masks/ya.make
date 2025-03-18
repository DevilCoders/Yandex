PY23_LIBRARY(infected_masks)

OWNER(
    g:antimalware
    g:antiwebspam
)

NO_WSHADOW()

PY_SRCS(
    NAMESPACE
    library.infected_masks
    masks_comptrie.pyx
)

IF (PYTHON2)
    PY_SRCS(
        NAMESPACE
        library.infected_masks
        infected_masks.pyx
    )
ENDIF()

PEERDIR(
    library/cpp/infected_masks
    contrib/python/six
)

END()
