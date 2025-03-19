GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(
    fs_job_output_saver.go
    hive_job_runner.go
    job_manager.go
    job_runner.go
    map_reduce_job_runner.go
    orchestrator.go
    process_executor.go
    py_spark_job_runner.go
    spark_job_runner.go
)

GO_TEST_SRCS(
    fs_job_output_saver_test.go
    hive_job_runner_test.go
    job_manager_test.go
    job_runner_test.go
    map_reduce_job_runner_test.go
    orchestrator_test.go
    py_spark_job_runner_test.go
    spark_job_runner_test.go
)

END()

RECURSE(
    gotest
    mocks
)
