OWNER(
    g:kp-sre
)

PY2_PROGRAM(awacs_stage_inserter)

PY_SRCS(
    MAIN main.py
    src/__init__.py
    src/AwacsBackend.py
    src/AwacsUpstream.py
    src/AwacsDomain.py
    src/logging_config.py
)

PEERDIR(
    infra/awacs/proto
    infra/nanny/nanny_rpc_client
    contrib/python/coloredlogs
    library/python/oauth
    contrib/python/PyYAML
)

END()
