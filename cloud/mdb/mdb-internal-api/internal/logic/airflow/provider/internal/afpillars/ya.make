GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    airflow.go
    converters.go
)

END()

RECURSE(gotest)
