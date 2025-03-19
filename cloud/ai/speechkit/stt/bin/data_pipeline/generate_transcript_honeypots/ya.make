PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN generate_transcript_honeypots.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/honeypots
    contrib/python/ujson
    library/python/nirvana
)

END()
