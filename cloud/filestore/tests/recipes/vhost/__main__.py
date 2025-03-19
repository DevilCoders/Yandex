import argparse
import os
import pathlib
import tempfile
import uuid

import yatest.common as common

from library.python.testing.recipe import declare_recipe, set_env

from cloud.filestore.config.vhost_pb2 import \
    TVhostAppConfig, TVhostServiceConfig

from cloud.filestore.tests.python.lib.common import \
    shutdown, get_restart_interval, get_restart_flag

from cloud.filestore.tests.python.lib.vhost import NfsVhost, wait_for_nfs_vhost
from cloud.storage.core.protos.endpoints_pb2 import EEndpointStorageType


PID_FILE_NAME = "nfs_vhost_recipe.pid"


def start(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--vhost-package-path", action="store", default=None)
    parser.add_argument("--verbose", action="store_true", default=False)
    parser.add_argument("--service", action="store", default=None)
    parser.add_argument("--restart-interval", action="store", default=None)
    parser.add_argument("--restart-flag", action="store", default=None)
    args = parser.parse_args(argv)

    vhost_binary_path = common.binary_path(
        "cloud/filestore/vhost/filestore-vhost")

    if args.vhost_package_path is not None:
        vhost_binary_path = common.build_path(
            "{}/usr/bin/filestore-vhost".format(args.vhost_package_path)
        )

    uid = str(uuid.uuid4())

    restart_interval = get_restart_interval(args.restart_interval)
    restart_flag = get_restart_flag(args.restart_flag, "qemu-ready-" + uid)

    endpoint_storage_dir = tempfile.gettempdir() + '/endpoints-' + uid
    pathlib.Path(endpoint_storage_dir).mkdir(parents=True, exist_ok=True)

    config = TVhostAppConfig()
    config.VhostServiceConfig.CopyFrom(TVhostServiceConfig())
    config.VhostServiceConfig.EndpointStorageType = EEndpointStorageType.ENDPOINT_STORAGE_FILE
    config.VhostServiceConfig.EndpointStorageDir = endpoint_storage_dir

    nfs_vhost = NfsVhost(
        binary_path=vhost_binary_path,
        cwd=common.output_path(),
        verbose=args.verbose,
        service=args.service,
        vhost_app_config=config,
        restart_interval=restart_interval,
        restart_flag=restart_flag,
    )

    nfs_vhost.start()

    with open(PID_FILE_NAME, "w") as f:
        f.write(str(nfs_vhost.pid))

    wait_for_nfs_vhost(nfs_vhost, nfs_vhost.port)

    if restart_interval:
        set_env("NFS_VHOST_ENDPOINT_STORAGE_DIR", endpoint_storage_dir)
        if restart_flag is not None:
            set_env("QEMU_SET_READY_FLAG", restart_flag)

    set_env("NFS_VHOST_PORT", str(nfs_vhost.port))


def stop(argv):
    if not os.path.exists(PID_FILE_NAME):
        return

    with open(PID_FILE_NAME) as f:
        pid = int(f.read())
        shutdown(pid)


if __name__ == "__main__":
    declare_recipe(start, stop)
