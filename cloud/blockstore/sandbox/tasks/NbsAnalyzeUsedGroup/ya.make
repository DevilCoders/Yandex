PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    plot_builder.py
    task.py
)

PEERDIR(
    contrib/python/plotly

    sandbox/sdk2
)

END()

RECURSE(
    core
)
