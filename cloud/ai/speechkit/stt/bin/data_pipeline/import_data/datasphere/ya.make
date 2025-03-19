OWNER(
    eranik
)

PY3_PROGRAM(datasphere)

PY_SRCS(
    operation.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
    cloud/ai/speechkit/stt/lib/data_pipeline/import_data/records
    cloud/ai/speechkit/stt/lib/utils/s3
    contrib/python/pandas
    contrib/python/numpy
    ml/nirvana/nope
    yt/python/client
)

PY_MAIN(nope.entry_point)

END()
