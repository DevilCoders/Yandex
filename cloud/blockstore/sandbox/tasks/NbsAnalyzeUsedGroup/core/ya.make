PROGRAM(nbs_analyze_used_group_core)

OWNER(g:cloud-nbs)

SRCS(
    main.cpp
    metrics.cpp
    options.cpp
    percentile_builder.cpp
    rate_calculator.cpp
    time_normalizer.cpp
    ydb_executer.cpp
)

PEERDIR(
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_params
    ydb/public/sdk/cpp/client/ydb_result
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
    ydb/public/sdk/cpp/client/ydb_value
    library/cpp/deprecated/atomic
)

END()
