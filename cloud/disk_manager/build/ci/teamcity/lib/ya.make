PY2_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
    kikimr/library/ci/teamcity_core
)

PY_SRCS(
    __init__.py
    sandbox_resource.py
)

END()
