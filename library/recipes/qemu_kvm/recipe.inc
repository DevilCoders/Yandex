#Forward test execution to qemu-vm environment
TAG(ya:dirty)
IF (NOT QEMU_ROOTFS_DIR)
   SET(QEMU_ROOTFS_DIR library/recipes/qemu_kvm/images/ubuntu-18.04-x86_64)
ENDIF()

IF (NOT QEMU_ROOTFS)
   SET(QEMU_ROOTFS ${QEMU_ROOTFS_DIR}/rootfs.img)
   DEPENDS(${QEMU_ROOTFS_DIR})
ENDIF()

IF (NOT QEMU_SSH_USER)
   SET(QEMU_SSH_USER qemu)
ENDIF()

IF (NOT QEMU_SSH_KEY)
   SET(QEMU_SSH_KEY library/recipes/qemu_kvm/images/keys/id_rsa)
   DATA(arcadia/library/recipes/qemu_kvm/images/keys)
ENDIF()

IF (NOT QEMU_SSH_PUBKEY)
   SET(QEMU_SSH_PUBKEY library/recipes/qemu_kvm/images/keys/id_rsa.pub)
   DATA(arcadia/library/recipes/qemu_kvm/images/keys)
ENDIF()

DEPENDS(
    library/recipes/qemu_kvm
    infra/qemu/bin
)

USE_RECIPE(
    library/recipes/qemu_kvm/qemu_kvm --rootfs $QEMU_ROOTFS --kernel $QEMU_KERNEL --ssh-key $QEMU_SSH_KEY --ssh-pubkey $QEMU_SSH_PUBKEY --ssh-user $QEMU_SSH_USER --mem $QEMU_MEM --proc $QEMU_PROC --qemu-options $QEMU_OPTIONS --use-as-test-host
)
REQUIREMENTS(kvm)
