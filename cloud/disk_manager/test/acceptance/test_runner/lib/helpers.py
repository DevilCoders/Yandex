from .errors import Error

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import YcpWrapper, make_ycp_engine

import logging
import paramiko
import socket


def check_ssh_connection(ip: str, profiler: common.Profiler) -> None:
    try:
        helpers = common.make_helpers(False)
        helpers.wait_until_instance_becomes_available_via_ssh(ip)
    except (paramiko.SSHException, socket.error) as e:
        profiler.add_ip(ip)
        raise Error(f'failed to connect to remote host {ip} via ssh: {e}')


def wait_for_block_device_to_appear(ip: str, block_device: str) -> None:
    helpers = common.make_helpers(False)
    helpers.wait_for_block_device_to_appear(ip, block_device)


def create_ycp(cluster_name: str,
               zone_id: str,
               ipc_type: str,
               logger: logging.Logger) -> YcpWrapper:
    cluster = get_cluster_test_config(cluster_name, zone_id)
    return YcpWrapper(cluster.name,
                      cluster.ipc_type_to_folder_desc[ipc_type],
                      logger,
                      make_ycp_engine(False))


def size_prettifier(size_bytes: int) -> str:
    if size_bytes % (1024 ** 4) == 0:
        return '%sTiB' % (size_bytes // 1024 ** 4)
    elif size_bytes % (1024 ** 3) == 0:
        return '%sGiB' % (size_bytes // 1024 ** 3)
    elif size_bytes % (1024 ** 2) == 0:
        return '%sMiB' % (size_bytes // 1024 ** 2)
    elif size_bytes % 1024 == 0:
        return '%sKiB' % (size_bytes // 1024)
    else:
        return '%sB' % size_bytes
