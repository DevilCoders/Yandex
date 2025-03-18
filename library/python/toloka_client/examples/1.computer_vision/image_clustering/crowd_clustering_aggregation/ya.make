OWNER(
    mr-fedulow
    g:toloka-crowd-instruments
)

PY3_LIBRARY()

PY_SRCS(
    aggregation.py
    data_structures.py
    crowd_clust.py
    MB_VDP.py
)

PEERDIR(
    contrib/python/pandas
    contrib/python/numpy
)

END()
