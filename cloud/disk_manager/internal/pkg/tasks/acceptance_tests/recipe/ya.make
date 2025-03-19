OWNER(g:cloud-nbs)

PY2_PROGRAM()

PY_SRCS(
    __main__.py
    node_launcher.py
)

PEERDIR(
    kikimr/ci/libraries
    library/python/testing/recipe
)

END()

RECURSE(
    init-db
    node
    tasks
)
