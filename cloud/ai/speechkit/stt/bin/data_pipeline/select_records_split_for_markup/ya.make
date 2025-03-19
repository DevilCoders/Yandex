PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN select_records_split_for_markup.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    contrib/python/ujson
    library/python/nirvana
)

END()
