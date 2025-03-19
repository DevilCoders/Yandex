import argparse
import os

import yatest.common as common

from library.python.testing.recipe import declare_recipe, set_env

from ydb.tests.library.harness.kikimr_cluster import kikimr_cluster_factory
from ydb.tests.library.harness.kikimr_config import KikimrConfigGenerator
from ydb.tests.library.harness.param_constants import kikimr_driver_path

from cloud.filestore.tests.python.lib.common import shutdown, get_restart_interval
from cloud.filestore.tests.python.lib.server import NfsServer, wait_for_nfs_server
from cloud.filestore.tests.python.lib.server_config import NfsServerConfigGenerator


PID_FILE_NAME = "local_kikimr_nfs_server_recipe.pid"
PDISK_SIZE = 32*1024*1024*1024


def start(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--kikimr-package-path", action="store", default=None)
    parser.add_argument("--nfs-package-path", action="store", default=None)
    parser.add_argument("--use-log-files", action="store_true", default=False)
    parser.add_argument("--verbose", action="store_true", default=False)
    parser.add_argument("--in-memory-pdisks", action="store_true", default=False)
    parser.add_argument("--restart-interval", action="store", default=None)
    args = parser.parse_args(argv)

    kikimr_binary_path = kikimr_driver_path()
    if args.kikimr_package_path is not None:
        kikimr_binary_path = common.build_path(
            "{}/Berkanavt/kikimr/bin/kikimr".format(args.kikimr_package_path)
        )

    kikimr_configurator = KikimrConfigGenerator(
        erasure=None,
        use_in_memory_pdisks=args.in_memory_pdisks,
        binary_path=kikimr_binary_path,
        has_cluster_uuid=False,
        static_pdisk_size=PDISK_SIZE,
        use_log_files=args.use_log_files)

    kikimr_cluster = kikimr_cluster_factory(configurator=kikimr_configurator)
    kikimr_cluster.start()

    kikimr_port = list(kikimr_cluster.nodes.values())[0].port

    set_env("KIKIMR_ROOT", kikimr_configurator.domain_name)
    set_env("KIKIMR_SERVER_PORT", str(kikimr_port))

    nfs_binary_path = common.binary_path(
        "cloud/filestore/server/filestore-server")

    if args.nfs_package_path is not None:
        nfs_binary_path = common.build_path(
            "{}/usr/bin/filestore-server".format(args.nfs_package_path)
        )

    nfs_configurator = NfsServerConfigGenerator(
        binary_path=nfs_binary_path,
        service_type="kikimr",
        verbose=args.verbose,
        domains_txt=kikimr_configurator.domains_txt,
        names_txt=kikimr_configurator.names_txt,
        kikimr_port=kikimr_port,
        restart_interval=get_restart_interval(args.restart_interval))

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
