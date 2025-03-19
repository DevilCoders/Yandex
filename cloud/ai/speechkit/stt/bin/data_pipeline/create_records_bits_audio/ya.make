PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN create_records_bits_audio.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data/ops
    contrib/python/ujson
    library/python/nirvana
)

END()
