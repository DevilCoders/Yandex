GO_LIBRARY()

OWNER(
    g:kikimr
    asmyasnikov
)

SRCS(
    coordination.go
    discovery.go
    driver.go
    ratelimiter.go
    retry.go
    scheme.go
    scripting.go
    table.go
    traces.go
    types.go
)

END()
