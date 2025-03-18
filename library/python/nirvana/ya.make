PY23_LIBRARY()

OWNER(mstebelev)

PY_SRCS(
    NAMESPACE
    nirvana
    __init__.py
    job_context.py
    mr_job_context.py
    snapshot.py
    util.py
    operation.py
    dc_context.py
)

PEERDIR(
    contrib/python/PyYAML
    yt/python/client
)

END()
