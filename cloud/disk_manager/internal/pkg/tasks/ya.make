OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    blank_task.go
    clear_ended_tasks_task.go
    headers.go
    registry.go
    runner.go
    runner_metrics.go
    scheduler.go
    scheduler_iface.go
    task.go
)

GO_TEST_SRCS(
    runner_test.go
    scheduler_test.go
    task_test.go
)

END()

RECURSE(
    config
    errors
    storage
)

RECURSE_FOR_TESTS(
    acceptance_tests
    mocks
    tasks_tests
    tests
)
