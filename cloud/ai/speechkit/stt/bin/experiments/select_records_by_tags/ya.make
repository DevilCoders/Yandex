PY3_PROGRAM()

OWNER(
    eranik
)

PY_SRCS(
    MAIN select_records_by_tags.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/experiments
    contrib/python/ujson
    library/python/nirvana
)

END()
