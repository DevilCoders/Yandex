PY23_LIBRARY()

OWNER(g:reactor)

PY_SRCS(
    TOP_LEVEL
    reactor_client/__init__.py
    reactor_client/client_base.py
    reactor_client/marshmallow_custom.py
    reactor_client/reaction_builders.py
    reactor_client/reactor_api.py
    reactor_client/reactor_objects.py
    reactor_client/reactor_schemas.py
    reactor_client/helper/__init__.py
    reactor_client/helper/artifact_instance.py
    reactor_client/helper/reaction/__init__.py
    reactor_client/helper/reaction/abstract_reaction.py
    reactor_client/helper/reaction/abstract_dynamic_reaction.py
    reactor_client/helper/reaction/blank_reaction.py
    reactor_client/helper/reaction/sandbox_reaction.py
    reactor_client/helper/reaction/yql_reaction.py
    reactor_client/helper/calculation/__init__.py
    reactor_client/helper/calculation/sandbox_dependency_resolver.py
    reactor_client/helper/calculation/yql_dependency_resolver.py
)

IF (PYTHON2)
    PEERDIR(
        contrib/python/enum34
        contrib/python/marshmallow/py2
    )
ELSE()
    PEERDIR(
        contrib/python/marshmallow/py3
    )
ENDIF()

PEERDIR(
    contrib/python/requests
    contrib/python/retry
    contrib/python/six
)

END()

RECURSE_FOR_TESTS(
    tests
)
