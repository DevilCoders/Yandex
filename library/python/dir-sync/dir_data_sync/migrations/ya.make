PY23_LIBRARY()

OWNER(g:tools-python)

PY_SRCS(
    NAMESPACE dir_data_sync.migrations
    __init__.py
    0001_initial.py
    0002_auto_20170309_1933.py
    0003_auto_20170316_1319.py
    0004_add_lang_to_org.py
    0005_auto_20170731_1043.py
    0006_remove_org_label_index.py
    0007_add_operatingmode_and_orgstatistics.py
    0008_init_operating_mode.py
    0009_add_mode_to_org.py
    0010_add_stats_to_every_org.py
    0011_add_org_status.py
)

END()
