OWNER(g:mstand)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE reports
    __init__.py
    compute_correlation.py
    conftest.py
    corr_context.py
    corr_struct.py
    exp_pairs.py
    gp_metric_scores.py
    metrics_compare.py
    # plot_metrics.py  # it needs bokeh lib
    report_helpers.py
    sensitivity_compare.py
)

PEERDIR(
    quality/yaqlib/yaqutils

    tools/mstand/adminka
    tools/mstand/correlations
    tools/mstand/experiment_pool
    tools/mstand/mstand_utils
    tools/mstand/project_root
    tools/mstand/user_plugins
)

END()

RECURSE_FOR_TESTS(
    tests
)
