PY3_LIBRARY()

OWNER(g:tasklet)

PY_SRCS(
    __init__.py
)

TASKLET_REG(BioTaskletImpl py ci.registry.junk.bsheludko.update_bio_beta_tasklet.lib:BioTaskletImpl)

PEERDIR(
    library/python/startrek_python_client
    ci/registry/junk/bsheludko/update_bio_beta_tasklet/proto
    search/priemka/yappy/services
    tasklet/services/yav
)

END()
