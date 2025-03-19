OWNER(g:cloud-nbs)

# https://st.yandex-team.ru/DEVTOOLSSUPPORT-18977#6285fbd36101de4de4e29f48
IF (SANITIZER_TYPE != "undefined" AND SANITIZER_TYPE != "memory")
    RECURSE(
        plugin
    )
ENDIF()

RECURSE(
    client
    cms
    fio
    infra-cms
    loadtest
    loadtest/remote
    notify
    python
    python_client
    recipes
    stats_aggregator_perf
)

IF (FUZZING)
    RECURSE(fuzzing)
ENDIF()
