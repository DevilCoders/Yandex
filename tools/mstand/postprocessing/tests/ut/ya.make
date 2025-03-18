PY3TEST()

OWNER(g:mstand)

SIZE(SMALL)

TEST_SRCS(
    tools/mstand/conftest.py

    tools/mstand/postprocessing/compute_criteria_ut.py
    tools/mstand/postprocessing/conftest.py
    tools/mstand/postprocessing/criteria_inputs_ut.py
    tools/mstand/postprocessing/postproc_engine_ut.py
    tools/mstand/postprocessing/postproc_helpers_ut.py
    tools/mstand/postprocessing/tsv_files_ut.py

    tools/mstand/postprocessing/scripts/buckets_ut.py
)

PEERDIR(
    tools/mstand/criterias
    tools/mstand/postprocessing
    tools/mstand/serp
    tools/mstand/session_squeezer
    tools/mstand/user_plugins
)

DATA(
    arcadia/tools/mstand/postprocessing/tests/ut/data
)

END()
