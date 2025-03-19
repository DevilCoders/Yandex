PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN process_aligner_output.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/experiments/records_utterances_split
    contrib/python/pydub
    library/python/nirvana
)

END()
