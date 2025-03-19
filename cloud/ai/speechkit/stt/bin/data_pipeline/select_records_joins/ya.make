PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN select_records_joins.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/select_records_joins
    contrib/python/ujson
    library/python/nirvana
)
END()
