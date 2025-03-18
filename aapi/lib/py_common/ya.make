PY2_LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/py_helpers
    contrib/python/futures
    mapreduce/yt/unwrapper
)

PY_SRCS(
    store.py
    consts.py
    vcs.py
    paths_tree.py
    svn.py
    uploader.py
)

END()
