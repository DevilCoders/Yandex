PY3TEST()

OWNER(g:mstand)

ENV(PYTEST_ADDOPTS=--seed 0)
SIZE(MEDIUM)
REQUIREMENTS(cpu:4)

TEST_SRCS(
    tools/mstand/conftest.py
    tools/mstand/criterias/criterias_ut_rnd.py
)

PEERDIR(
    contrib/python/numpy
    tools/mstand/criterias
)

END()
