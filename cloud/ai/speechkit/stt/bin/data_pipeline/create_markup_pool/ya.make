PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN create_markup_pool.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/toloka
    cloud/ai/speechkit/stt/lib/data_pipeline/user_bits_urls
    library/python/nirvana
)

END()
