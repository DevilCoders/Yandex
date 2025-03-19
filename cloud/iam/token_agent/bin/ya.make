PY2_PROGRAM(token-agent-init)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/PyJWT
    contrib/python/PyYAML
    contrib/python/requests
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1
)

PY_MAIN(token-agent-init)

PY_SRCS(
    TOP_LEVEL
    token-agent-init.py
)

END()
