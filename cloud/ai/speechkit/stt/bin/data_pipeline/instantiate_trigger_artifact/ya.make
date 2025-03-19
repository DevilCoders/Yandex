PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN instantiate_trigger_artifact.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/reactor
    library/python/nirvana
)

END()
