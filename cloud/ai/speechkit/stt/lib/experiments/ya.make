PY3_LIBRARY()

OWNER(
    eranik
)

PY_SRCS(
    __init__.py
    processing/__init__.py
    processing/select_records.py
    queries/__init__.py
    queries/records_by_tags_marks.py
    queries/records_filters.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/eval
)

END()
