OWNER(g:mstand)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE postprocessing.scripts
    __init__.py
    buckets.py
    combine_metric_values.py
    echo.py
    lamps_postproc.py
    linearization.py
    postproc_samples.py
    random_drop.py
    row_length.py
)

PEERDIR(
    tools/mstand/experiment_pool
    tools/mstand/user_plugins
)

END()
