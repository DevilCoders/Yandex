OWNER(g:mdb)

PY3_PROGRAM(dbaas_internal_api_dev)

STYLE_PYTHON()

PY_SRCS(
    MAIN
    run.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api
    cloud/mdb/dbaas-internal-api-image/tests/providers
    # Mostly for debug reasons
    # If your need jump into A-python-binary
    #
    # export Y_PYTHON_ENTRY_POINT=:repl
    # set -x Y_PYTHON_ENTRY_POINT :repl
    contrib/python/ipython
)

# import_test fails, cause
# we try initialize APP and it fails on logging init

NO_CHECK_IMPORTS()

END()

RECURSE(
    dbaas_internal_api
    func_tests
    recipe
    reindexer
    tests
    uwsgi
    yo_test
)
