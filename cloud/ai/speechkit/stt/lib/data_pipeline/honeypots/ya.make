PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/lib/python/text/mutator
)

PY_SRCS(
    check_transcript/__init__.py
    check_transcript/generate.py
    transcript/__init__.py
    transcript/generate.py
    __init__.py
)

END()
