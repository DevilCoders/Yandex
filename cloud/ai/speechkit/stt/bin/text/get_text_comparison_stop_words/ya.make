PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN get_text_comparison_stop_words.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/text/text_comparison_stop_words
    library/python/nirvana
)

END()
