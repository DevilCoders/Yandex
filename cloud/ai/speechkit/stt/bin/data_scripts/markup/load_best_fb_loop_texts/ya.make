PY3_PROGRAM()

PY_SRCS(
    MAIN load_best_fb_loop_texts.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
    library/python/nirvana
)

END()
