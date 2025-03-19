PY3_LIBRARY()

OWNER(g:cloud-ps)

PEERDIR(
    cloud/ps/gore/scripts/sanbox_jobs/config
    contrib/python/requests
    
)

PY_SRCS(
    fetcher.py
    kicker.py
    retry.py
)

END()
