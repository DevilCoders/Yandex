OWNER(g:mstand)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE correlations
    __init__.py
    chi_squared.py
    corr_pearson.py
    corr_spearmanr.py
    correlation_api.py
)

PEERDIR(
    contrib/python/scikit-learn

    tools/mstand/experiment_pool
    tools/mstand/mstand_utils
)

END()

RECURSE_FOR_TESTS(
    tests
)
