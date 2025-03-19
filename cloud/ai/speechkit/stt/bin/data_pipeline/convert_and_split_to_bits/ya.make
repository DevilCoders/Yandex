PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN convert_and_split_to_bits.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/records_splitting
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    contrib/python/pydub
    contrib/python/ujson
    library/python/nirvana
)

END()
