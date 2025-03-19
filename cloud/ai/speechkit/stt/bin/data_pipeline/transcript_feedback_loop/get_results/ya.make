PY3_PROGRAM()

PY_SRCS(
    MAIN get_results.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/obfuscate
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_quality
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data/ops
    library/python/nirvana
)

END()
