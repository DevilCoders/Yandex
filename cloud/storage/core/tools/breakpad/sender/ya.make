PY2_PROGRAM(yc-storage-breakpad-sender)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/storage/core/tools/breakpad
)

PY_MAIN(cloud.storage.core.tools.breakpad.crash_processor:main)

END()
