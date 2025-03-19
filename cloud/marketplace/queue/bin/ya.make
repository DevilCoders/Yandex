OWNER(g:cloud-marketplace)

PY3_PROGRAM(yc_marketplace_queue)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    cloud/marketplace/queue/yc_marketplace_queue
    cloud/marketplace/common/yc_marketplace_common
    cloud/bitbucket/python-common
)

END()
