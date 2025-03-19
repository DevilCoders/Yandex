OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    batch.py
    dataset.py
    datasource.py
    reader.py
    sample.py
    save_yt_table.py
    utils.py
)

NO_CHECK_IMPORTS()

END()
