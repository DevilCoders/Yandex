GO_LIBRARY()

SRCS(
    adapter.go
    billing_account.go
    common.go
    configurator.go
    cumulative.go
    dict_stores.go
    dicts.go
    errors.go
    presenter.go
    push.go
    retry.go
    sku.go
    uniq.go
    util.go
)

GO_TEST_SRCS(
    adapter_test.go
    billing_account_test.go
    common_test.go
    configurator_test.go
    cummulative_test.go
    dicts_test.go
    mock_SchemeSession_test.go
    push_test.go
    retry_test.go
    sku_test.go
    uniq_test.go
)

END()

RECURSE(gotest)
