GO_LIBRARY()

OWNER(g:mdb)

SRCS(airflow.go)

END()

RECURSE(
    afmodels
    mocks
    provider
)
