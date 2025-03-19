PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN client_markup.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_cost
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/utils/s3
    library/python/nirvana
)

END()
