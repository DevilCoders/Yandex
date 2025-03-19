GO_LIBRARY()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/mdb-maintenance/configs)

SRCS(
    cal.go
    models.go
)

GO_TEST_SRCS(
    cal_test.go
    config_test.go
)

END()

RECURSE(gotest)
