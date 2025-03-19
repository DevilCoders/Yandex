PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN create_records_joins.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/join
    contrib/python/ujson
    library/python/nirvana
)

END()
