import argparse
import os

import yatest.common as common

from library.python.testing.recipe import declare_recipe, set_env

from cloud.filestore.tests.python.lib.common import shutdown
from cloud.filestore.tests.python.lib.server import NfsServer, wait_for_nfs_server
from cloud.filestore.tests.python.lib.server_config import NfsServerConfigGenerator


PID_FILE_NAME = "local_service_nfs_server_recipe.pid"


def start(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--nfs-package-path", action="store", default=None)
    parser.add_argument("--verbose", action="store_true", default=False)
    parser.add_argument("--service", action="store", default="null")
    args = parser.parse_args(argv)

    nfs_binary_path = common.binary_path(
        "cloud/filestore/server/filestore-server")

    if args.nfs_package_path is not None:
        nfs_binary_path = common.build_path(
            "{}/usr/bin/filestore-server".format(args.nfs_package_path)
        )

    nfs_configurator = NfsServerConfigGenerator(
        binary_path=nfs_binary_path,
        service_type=args.service,
        verbose=args.verbose)

    nfs_server = NfsServer(configurator=nfs_configurator)
    nfs_server.start()

    with open(PID_FILE_NAME, "w") as f:
        f.write(str(nfs_server.pid))

    wait_for_nfs_server(nfs_server, nfs_configurator.port)

    set_env("NFS_SERVER_PORT", str(nfs_configurator.port))
    set_env("NFS_MON_PORT", str(nfs_configurator.mon_port))


def stop(argv):
    if not os.path.exists(PID_FILE_NAME):
        return

    with open(PID_FILE_NAME) as f:
        pid = int(f.read())
        shutdown(pid)


if __name__ == "__main__":
    declare_recipe(start, stop)
