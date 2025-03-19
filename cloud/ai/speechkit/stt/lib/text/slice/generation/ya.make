PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/text/slice/application
    cloud/ai/speechkit/stt/lib/text/text_comparison_stop_words
)

END()
