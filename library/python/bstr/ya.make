PY23_LIBRARY()

OWNER(pg)

NO_CHECK_IMPORTS(pygments.sphinxext)

IF (OS_LINUX)
    PEERDIR(
        contrib/python/directio
    )
ENDIF ()

PEERDIR(
    contrib/python/PyYAML
    contrib/python/Pygments
    contrib/python/fallocate
    contrib/python/humanfriendly
    contrib/python/six
    contrib/python/tabulate
    contrib/python/watchdog
    contrib/python/pyre2
    contrib/python/parsedatetime
    contrib/python/marisa_trie
    contrib/python/ujson
    library/python/init_log
    library/python/oauth
    library/python/yt
    yt/python/client
    contrib/python/Flask
)

IF(PYTHON2)
    PEERDIR(
        contrib/deprecated/python/subprocess32
    )
ENDIF()

PY_SRCS(
    main.py
    common.py
    push.py
    pull.py
    rand.py
    stress.py
    stages_logger.py
    compress.py
    agent.py
)

END()

