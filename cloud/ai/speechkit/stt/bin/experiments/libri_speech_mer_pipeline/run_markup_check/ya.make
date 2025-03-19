PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN run_markup_check.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/experiments/libri_speech_mer_pipeline
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    library/python/nirvana
)

END()
