PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    task.py
    wrapper.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/find-perf-bottlenecks/lib
    cloud/blockstore/tools/analytics/find-perf-bottlenecks/ytlib

    sandbox/projects/cloud/blockstore/resources
)

END()
