PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/utils/arcadia
)

PY_SRCS(
    __init__.py
    arcadia.py
)

END()
