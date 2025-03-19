PY3_PROGRAM(yc-nbs-ci-deploy-packages)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/z2
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common

    cloud/blockstore/tools/ci/deploy_packages/lib
)

END()

RECURSE(
    lib
)
