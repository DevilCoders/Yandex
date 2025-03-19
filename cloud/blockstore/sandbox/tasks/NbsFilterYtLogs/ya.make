PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    task.py
    wrapper.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/filter-yt-logs/lib
)

END()
