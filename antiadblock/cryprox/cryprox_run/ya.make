PY2_PROGRAM()

OWNER(g:antiadblock)

PY_SRCS(
    run.py
)
IF (OS_LINUX)
    PEERDIR(
        contrib/python/yappi
        contrib/python/tornado/tornado-4
        contrib/python/python-prctl
        antiadblock/cryprox/cryprox
        antiadblock/libs/tornado_redis
    )
ELSE()
    PEERDIR(
        contrib/python/yappi
        contrib/python/tornado/tornado-4
        antiadblock/cryprox/cryprox
        antiadblock/libs/tornado_redis
    )
ENDIF()

PY_MAIN(antiadblock.cryprox.cryprox_run.run:main)

END()
