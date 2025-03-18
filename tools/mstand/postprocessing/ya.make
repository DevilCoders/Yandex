OWNER(g:mstand)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE postprocessing
    __init__.py
    compute_criteria.py
    compute_criteria_single.py
    compute_criteria_test_lib.py
    criteria_api.py
    criteria_context.py
    criteria_inputs.py
    criteria_read_mode.py
    criteria_utils.py
    postproc_api.py
    postproc_context.py
    postproc_engine.py
    postproc_helpers.py
    tsv_files.py
)

PEERDIR(
    quality/yaqlib/yaqutils

    tools/mstand/adminka
    tools/mstand/experiment_pool
    tools/mstand/metrics_api
    tools/mstand/mstand_utils
    tools/mstand/postprocessing/scripts
    tools/mstand/reports
)

END()

RECURSE_FOR_TESTS(
    tests
)
