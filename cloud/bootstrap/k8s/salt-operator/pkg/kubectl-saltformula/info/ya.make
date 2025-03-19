GO_LIBRARY()

SRCS(
    info.go
    job_info.go
    pod_info.go
    saltformula_info.go
)

END()

RECURSE(testdata)
