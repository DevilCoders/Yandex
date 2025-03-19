#!/usr/bin/python
import subprocess
import shlex

# For info https://devicehunt.com/view/type/pci/vendor/10DE/device/1DB6
VENDOR_ID = "10de" # Nvidia
DEVICE_ID = "1db6" # Tesla V100 PCIe 32GB


def _run_cmd(cmd):
    """Run any cmd using SP"""
    s_p = subprocess.Popen(args=shlex.split(cmd), stdout=subprocess.PIPE)
    data_stdout, _ = s_p.communicate()
    return data_stdout.decode(encoding="utf-8")


def _get_gpu():
    """Get GPU cards on host"""
    gpu = {}
    cmd = "lspci -d {}:{}" .format(VENDOR_ID, DEVICE_ID)
    raw_data = [device for device in _run_cmd(cmd).split("\n") if device]
    if not raw_data:
        gpu["available"] = False
        return gpu
    gpu["available"] = True
    gpu["gpu_devices"] = [pci_id.split()[0] for pci_id in raw_data if pci_id]
    return gpu


def main():
    GPU = {"GPU": _get_gpu()}
    return GPU

if __name__ == '__main__':
    main()
