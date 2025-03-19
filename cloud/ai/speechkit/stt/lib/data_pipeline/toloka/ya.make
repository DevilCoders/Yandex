PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PEERDIR(
    cloud/ai/lib/python/iterators
    cloud/ai/lib/python/log
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/files
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/text/re
    library/python/toloka-kit
)

PY_SRCS(
    __init__.py
    api.py
    task_bit_combiner.py
)

END()
