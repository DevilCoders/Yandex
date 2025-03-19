PY3_LIBRARY()

OWNER(uplink)

PEERDIR(
    contrib/python/requests
    cloud/ps/startrack_reopen_ps/config
)

PY_SRCS(
    check.py
    startrack.py
)

END()