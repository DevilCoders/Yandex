PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN obfuscate.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/obfuscate
    library/python/nirvana
    contrib/python/ujson
)

END()
