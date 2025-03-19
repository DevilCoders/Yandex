PY2_PROGRAM(teamcity-build-packages)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/disk_manager/build/ci/sandbox
    cloud/disk_manager/build/ci/teamcity_new/common
)

END()

