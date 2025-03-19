PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN upload_obfucated_bits.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/utils/s3
    contrib/python/ujson
    library/python/nirvana
)

END()
