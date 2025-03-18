OWNER(g:mstand)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE criterias
    __init__.py
    auto.py
    bootstrap.py
    mannwhitneyu.py
    ranksums.py
    ttest.py
    tw_lr.py
)

PEERDIR(
    contrib/python/numpy
    contrib/python/pandas
    contrib/python/scikit-learn
    contrib/python/scipy

    quality/yaqlib/yaqutils

    tools/mstand/mstand_utils
    tools/mstand/postprocessing
    tools/mstand/user_plugins
)

END()

RECURSE_FOR_TESTS(
    tests
)
