PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb-dataproc)

PY_NAMESPACE(tests)

PEERDIR(
    contrib/python/argon2-cffi
    contrib/python/behave
    contrib/python/deepdiff
    contrib/python/humanfriendly
    contrib/python/PyHamcrest
    contrib/python/PyNaCl
    contrib/python/pytest
    contrib/python/pyOpenSSL
    contrib/python/PyYAML
    cloud/mdb/dataproc-infra-tests/tests/helpers
    cloud/mdb/internal/python/ipython_repl
)

PY_SRCS(
    NAMESPACE tests
    configuration.py
    env_control.py
    environment.py
    local_config.py
    logs.py
    shell.py
)

END()

RECURSE(
    helpers
)
