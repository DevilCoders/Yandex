PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/mstand_utils/mstand_misc_helpers_ut.py
    tools/mstand/mstand_utils/mstand_module_helpers_ut.py
    tools/mstand/mstand_utils/mstand_paths_utils_ut.py
    tools/mstand/mstand_utils/mstand_tables_ut.py
    tools/mstand/mstand_utils/stat_helpers_ut.py
    tools/mstand/mstand_utils/testid_helpers_ut.py
)

PEERDIR(
    contrib/python/numpy
    contrib/python/scipy
    quality/yaqlib/yaqutils
    tools/mstand/mstand_utils
)

DATA(
    arcadia/tools/mstand/mstand_utils/tests/data/
)

SIZE(SMALL)

END()
