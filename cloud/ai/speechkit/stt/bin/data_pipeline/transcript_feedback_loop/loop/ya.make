PY3_PROGRAM()

PY_SRCS(
    MAIN loop.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
    library/python/nirvana
)

END()
