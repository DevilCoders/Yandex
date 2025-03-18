OWNER(
    dmitko
    g:yatool
)

EXECTEST()
# Run qemu with custom device list, and check that devices exists
RUN(
    NAME check_devices
    /usr/bin/lspci -d "1af4:1050"
    STDOUT ${TEST_OUT_ROOT}/stdout.log
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/stdout.log
)

SET(QEMU_SSH_USER root)
SET(QEMU_OPTIONS \'-device virtio-gpu-pci -device virtio-gpu-pci\')
SET(QEMU_ROOTFS_DIR infra/environments/rtc-xenial-gpu/release/vm-image)
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()
