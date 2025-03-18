OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/ujson
    contrib/python/numpy
    contrib/python/scipy
    contrib/python/Jinja2
    yt/python/client
    quality/logs/baobab/api/python/baobab
    quality/yaqlib/pytlib
    quality/yaqlib/yaqutils
    tools/mstand/mstand_enums
    tools/mstand/project_root
)

PY_SRCS(
    NAMESPACE mstand_utils
    __init__.py
    mstand_def_values.py
    args_helpers.py
    baobab_helper.py
    checkout_helpers.py
    client_yt.py
    log_helpers.py
    mstand_misc_helpers.py
    mstand_module_helpers.py
    mstand_paths_utils.py
    mstand_tables.py
    stat_helpers.py
    testid_helpers.py
    wiki_helpers.py
    yt_helpers.py
    yt_options_struct.py

    tests/data/test_import.py  # It needs for some tests only
)

END()

RECURSE_FOR_TESTS(
    tests
)
