PY3_PROGRAM()

PY_SRCS(
    MAIN create_pools.py
)

PEERDIR(
    cloud/ai/lib/python/log
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
    library/python/nirvana
)

END()
