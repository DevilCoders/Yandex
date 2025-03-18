PACKAGE()

OWNER(dmtrmonakhov dmitko)

FROM_SANDBOX(860767875 OUT_NOAUTO
    opt/qemu-kvm
    opt/qemu-lite/usr/bin/qemu-system-x86_64
    opt/qemu-lite/usr/bin/qemu-img
    opt/qemu-lite/usr/share/qemu-lite/qemu/efi-e1000.rom
    opt/qemu-lite/usr/share/qemu-lite/qemu/efi-virtio.rom
    opt/qemu-lite/usr/share/qemu-lite/qemu/sgabios.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/kvmvapic.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/bios-256k.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/linuxboot.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/linuxboot_dma.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/vgabios-stdvga.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/vgabios.bin
    opt/qemu-lite/usr/share/qemu-lite/qemu/multiboot.bin
)

END()
