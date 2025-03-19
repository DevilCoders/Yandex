PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN calculate_tags_statistics.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    library/python/nirvana
)

END()
