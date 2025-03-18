PY3_LIBRARY()

OWNER(g:tasklet)

PY_SRCS(
    __init__.py
)

TASKLET_REG(TaskletImpl py ci.registry.junk.abezhin.update_beta_tasklet.lib:TaskletImpl)

PEERDIR(
    library/python/startrek_python_client
    ci/registry/junk/abezhin/update_beta_tasklet/proto
    search/priemka/yappy/services
    tasklet/services/yav
)

END()