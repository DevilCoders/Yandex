PY3_LIBRARY()

OWNER(g:tasklet)

PY_SRCS(
    __init__.py
    nirvana.py
)


TASKLET_REG(CIFinalize py ci.registry.junk.abezhin.ci_final_parser.lib:FinalParserImpl)


PEERDIR(
    contrib/python/requests
    ci/registry/junk/abezhin/ci_final_parser/proto
    sandbox/sdk2
    tasklet/services/yav
    ml/pulsar/python-package
    yt/python/client
)


END()