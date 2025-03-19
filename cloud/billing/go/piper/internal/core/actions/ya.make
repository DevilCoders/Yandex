GO_LIBRARY()

SRCS(
    actions_easyjson.go
    e2e.go
    enrich_source.go
    identity.go
    metric_labels.go
    pools.go
    presenter.go
    push.go
    aggregate.go
    sku.go
    sku_tools.go
    time.go
    tooling.go
    validating.go
)

GO_TEST_SRCS(
    e2e_test.go
    enrich_source_test.go
    generate_test.go
    identity_test.go
    presenter_test.go
    push_test.go
    aggregate_test.go
    sku_test.go
    sku_tools_test.go
    validating_test.go
)

END()

RECURSE(
    gotest
    mocks
)
