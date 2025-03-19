PACKAGE()

OWNER(g:cloud-nbs)

FROM_SANDBOX(
    FILE
    3357454439
    AUTOUPDATED qemu_binary
    RENAME RESOURCE
    OUT_NOAUTO qemu-bin.tar.gz
)

END()
