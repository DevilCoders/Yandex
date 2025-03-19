PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    contrib/python/pydub
)

PY_SRCS(
    __init__.py
    obfuscate.py
)

END()

