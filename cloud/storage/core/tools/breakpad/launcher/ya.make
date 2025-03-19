PY2_PROGRAM(yc-storage-breakpad-launcher)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/storage/core/tools/breakpad
)

PY_MAIN(cloud.storage.core.tools.breakpad.launcher:main)

END()
