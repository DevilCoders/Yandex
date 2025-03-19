PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN fetch_texts.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/experiments/libri_speech_mer_pipeline
)

END()
