PY3TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    test.py
)

PEERDIR(
    contrib/python/pandas
    library/python/resource
    antiadblock/tasks/money_monitoring/lib
)

RESOURCE(
    data/tn1.csv tn/1 # ANTIADBALERTS-34
    data/tn2.csv tn/2 # ANTIADBALERTS-8
    data/tn3.csv tn/3 # ANTIADBALERTS-2
    data/tn4.csv tn/4 # ANTIADBALERTS-1
    data/tp1.csv tp/1 # turbo
)

SIZE(SMALL)

END()
