PY3TEST()

OWNER(g:mstand)

ENV(PYTEST_ADDOPTS=--seed 0)
SIZE(MEDIUM)
REQUIREMENTS(cpu:4)

TEST_SRCS(
    tools/mstand/conftest.py
    tools/mstand/postprocessing/compute_criteria_synthetic_ut_rnd.py
    tools/mstand/postprocessing/conftest.py
)

PEERDIR(
    contrib/python/pandas
    contrib/python/scikit-learn

    tools/mstand/criterias
    tools/mstand/postprocessing
)

END()
