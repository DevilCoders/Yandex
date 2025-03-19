OWNER(g:cloud-nbs)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    disk_policy.py
    errors.py
    helpers.py
    instance_policy.py
    test_cases.py
)

PEERDIR(
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/ycp
)

END()
