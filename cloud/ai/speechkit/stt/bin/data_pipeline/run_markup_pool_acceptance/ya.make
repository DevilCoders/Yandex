PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN run_markup_pool_acceptance.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/obfuscate
    cloud/ai/speechkit/stt/lib/data_pipeline/join
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_quality
    contrib/python/ujson
    library/python/nirvana
)

END()
