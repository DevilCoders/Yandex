PY23_LIBRARY(nirvana_api)

OWNER(mihajlova)

IF (PYTHON2)
    PEERDIR(
        contrib/python/enum34
    )
ENDIF()

PEERDIR(
    library/python/nirvana_api/blocks
    contrib/python/graphviz
    contrib/python/requests
    contrib/python/six
)

PY_SRCS(
    NAMESPACE nirvana_api
    api.py
    fun.py
    data_type.py
    execution_state.py
    json_rpc.py
    highlevel_api.py
    parameter_classes.py
    secrets.py
    workflow.py
    workflow_description.py
    __init__.py
)

END()

RECURSE_FOR_TESTS(
    tests
    tests/manual
    ut
)
