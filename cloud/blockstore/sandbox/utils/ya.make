PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    arc_helper.py
    artifact_helper.py
    config_helper.py
    constants.py
)

PEERDIR(
    contrib/python/deepmerge
    contrib/python/jsonschema
    contrib/python/pyaml

    library/python/retry

    sandbox/common/errors
    sandbox/common/types
    sandbox/projects/common/teamcity
    sandbox/projects/common/vcs
    sandbox/sdk2
)

END()
