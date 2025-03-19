OWNER(g:mdb)

RECURSE(
    populate_table
    compute
    grpcutil
    ipython_repl
    pg_create_users
    pytest
    logs
    loadbalancer
    lockbox
    managed_kubernetes
    managed_postgresql
    vault
    vpc
    query_conf
    yandex_team
)

RECURSE_FOR_TESTS(
    test
)
