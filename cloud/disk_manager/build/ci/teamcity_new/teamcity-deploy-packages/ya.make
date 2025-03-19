PY2_PROGRAM(teamcity-deploy-packages)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/disk_manager/build/ci/z2
    cloud/disk_manager/build/ci/teamcity_new/common
)

END()

