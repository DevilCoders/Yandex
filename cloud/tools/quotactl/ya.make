PY2_PROGRAM()

OWNER(bgleb)

PEERDIR(
    contrib/python/click
    contrib/python/prettytable
    contrib/python/ipython
    cloud/bitbucket/private-api/yandex/cloud/priv/serverless/functions/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/quota
)

PY_MAIN(quotactl)

PY_SRCS(
    TOP_LEVEL
    quotactl.py
)

END()
