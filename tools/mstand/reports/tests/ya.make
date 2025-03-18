PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/reports/compute_correlation_ut.py
    tools/mstand/reports/corr_struct_ut.py
    tools/mstand/reports/gp_metric_scores_ut.py
    tools/mstand/reports/metrics_compare_ut.py
    # tools/mstand/reports/plot_metrics_ut.py  # it needs bokeh lib
    tools/mstand/reports/report_helpers_ut.py
    tools/mstand/reports/sensitivity_compare_ut.py
)

PEERDIR(
    tools/mstand/reports
)

DATA(
    arcadia/tools/mstand/adminka/tests/data
    arcadia/tools/mstand/reports/tests/data
    arcadia/tools/mstand/templates
    arcadia/tools/mstand/tests/ut_data
)

SIZE(SMALL)

END()
