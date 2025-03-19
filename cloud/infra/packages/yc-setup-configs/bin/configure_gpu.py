#!/usr/bin/env python3

import argparse
import logging
import socket
import sys

from ycinfra import (
    InventoryApi,
    write_file_content,
)

logging.basicConfig(level=logging.DEBUG, format="%(levelname)s - %(message)s")

# For info https://devicehunt.com/view/type/pci/vendor/10DE/device/1DB6 Tesla V100 PCIe 32GB
#          https://devicehunt.com/view/type/pci/vendor/10DE/device/1DB5 Tesla V100 SXM2 32GB
#          https://devicehunt.com/view/type/pci/vendor/10DE/device/20B0 A100 SXM4 40GB
#          https://pci-ids.ucw.cz/read/PC/10de/20b2                     A100 SXM4 80GB
#          https://pci-ids.ucw.cz/read/PC/10de/1eb8                     Tesla T4
#          https://devicehunt.com/view/type/pci/vendor/10DE/device/1AF1 NVswitch
VENDOR_ID = "10de"  # Nvidia vendor ID
DEVICE_ID_ARRAY = ["1db6", "1db5", "20b0", "20b2", "1eb8", "1af1"]
HW_GPU_PLATFORMS = ["nvidia-tesla-gv100gl-pcie-32G",
                    "nvidia-tesla-gv100gl-sxm2-32G",
                    "a100",
                    "nvidia-ampere-a100-sxm4-40G",
                    "nvidia-ampere-a100-sxm4-80G",
                    "nvidia-tesla-t4-pcie-16G",
                    ]
VGPU_PLATFORMS = ["nvidia-grid-v100d-8q",
                  "nvidia-grid-v100d-8q-6230",
                  "nvidia-grid-v100d-16q",
                  ]
# NOTE(zasimov-a): VGPU_PT_PLATFORM list contains GPU platforms that should be
# configured with 'iommu=pt'. See https://st.yandex-team.ru/CLOUD-101014 and
# related tickets for more details.
# VGPU_PT_PLATFORM are mapped to 'vgpu-pt' GPU_PLATFORM type.
VGPU_PT_PLATFORMS = ["nvidia-grid-a100x-5c"]
HW_GPU_PLATFORM = "hwgpu"
VGPU_PLATFORM = "vgpu"
VGPU_PT_PLATFORM = "vgpu-pt"
UNKNOWN_GPU_PLATFORM = "unknown"
GPU_PLATFORM_FILE = "/etc/yc/infra/compute_gpu_platform.yaml"


def _configure_vfio_rules():
    modprobe_gpu_vfio_file = "/etc/modprobe.d/gpu_vfio.conf"
    logging.debug("Write modprobe rules: %s", modprobe_gpu_vfio_file)
    device_vendor_ids = ",".join(["{}:{}".format(VENDOR_ID, device_id) for device_id in DEVICE_ID_ARRAY])
    modprobe_gpu_vfio_file_content = "options vfio-pci ids={}".format(device_vendor_ids)
    write_file_content(modprobe_gpu_vfio_file, modprobe_gpu_vfio_file_content)


def _configure_blacklist_nouveau_rules():
    blacklist_file = "/etc/modprobe.d/blacklist-nouveau.conf"
    blacklist_file_content = "blacklist nouveau"
    logging.debug("Write blacklist rules: %s", blacklist_file)
    write_file_content(blacklist_file, blacklist_file_content)


def _configure_vgpu_nvidia_rules():
    # Use "Fixed share scheduler with the default time slice length"
    # See: https://docs.nvidia.com/grid/latest/grid-vgpu-user-guide/index.html#rmpvmrl-registry-key
    modprobe_vgpu_nvidia_file = "/etc/modprobe.d/nvidia.conf"
    modprobe_vgpu_nvidia_file_content = 'options nvidia NVreg_RegistryDwords="RmPVMRL=0x11"'
    logging.debug("Write modprobe rules: %s", modprobe_vgpu_nvidia_file)
    write_file_content(modprobe_vgpu_nvidia_file, modprobe_vgpu_nvidia_file_content)


def configure_gpu(gpu_platform):
    logging.debug("Current GPU platform for host: %s", gpu_platform)
    if gpu_platform == "None":
        return None

    _configure_blacklist_nouveau_rules()

    if gpu_platform in HW_GPU_PLATFORMS:
        logging.debug("This is hardware GPU host, let's configure it..")
        _configure_vfio_rules()

    elif gpu_platform in VGPU_PLATFORMS:
        logging.debug("This is vGPU host, let's configure it..")
        _configure_vgpu_nvidia_rules()
    return None


def get_gpu_type(gpu_platform):
    if gpu_platform in HW_GPU_PLATFORMS:
        return HW_GPU_PLATFORM
    if gpu_platform in VGPU_PT_PLATFORMS:
        return VGPU_PT_PLATFORM
    if gpu_platform in VGPU_PLATFORMS:
        return VGPU_PLATFORM
    return UNKNOWN_GPU_PLATFORM


def parse_args():
    parser = argparse.ArgumentParser(description="GPU topology")
    subparsers = parser.add_subparsers(dest="action")
    subparsers.add_parser("generate", help="Save GPU topology to file")
    subparsers.add_parser("configure", help="Save GPU topology to file and configure drivers")
    subparsers.add_parser("get-gpu-type", help="Output type of gpu platform hwGPU/vGPU")

    args = parser.parse_args()
    if args.action is None:
        parser.print_help()
        return None

    return args


if __name__ == "__main__":
    args = parse_args()
    if not args:
        sys.exit(1)
    host_gpu_platform = InventoryApi().get_gpu_platform(socket.gethostname())
    if not host_gpu_platform:
        logging.error("GPU platform could not be obtained from Infra-proxy. Exiting.")
        sys.exit(1)
    if args.action == "get-gpu-type":
        print(get_gpu_type(host_gpu_platform))
        sys.exit(0)

    if args.action in ["generate", "configure"]:
        logging.info("Writing gpu platform to file '%s'", host_gpu_platform)
        try:
            write_file_content(GPU_PLATFORM_FILE, host_gpu_platform)
        except OSError as ex:
            print(ex)
            sys.exit(1)

    if args.action == "configure":
        try:
            configure_gpu(host_gpu_platform)
        except OSError as ex:
            print(ex)
            sys.exit(1)
