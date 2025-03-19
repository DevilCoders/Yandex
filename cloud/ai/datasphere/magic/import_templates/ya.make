OWNER(eranik g:cloud-asr)

PY3_PROGRAM(import_templates)

PEERDIR(
    ml/nirvana/nope
    yt/python/client
    cloud/ai/speechkit/stt/lib/utils/s3
    cloud/ai/datasphere/lib/stt/text_templates/python_package/lib
)

PY_SRCS(
    operation.py
)

PY_MAIN(nope.entry_point)

END()
