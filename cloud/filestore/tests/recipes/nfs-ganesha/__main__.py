import argparse
import os
import uuid

import yatest.common as common

from library.python.testing.recipe import declare_recipe, set_env

from cloud.filestore.tests.python.lib.common import \
    shutdown, get_restart_interval, get_restart_flag

from cloud.filestore.tests.python.lib.nfs_ganesha import \
    create_nfs_ganesha_mem, create_nfs_ganesha_vfs, create_nfs_ganesha_yfs, \
    wait_for_nfs_ganesha

from cloud.filestore.tests.python.lib.client import NfsCliClient


BLOCK_SIZE = 4*1024
BLOCKS_COUNT = 1000

PID_FILE_NAME = "nfs_ganesha_recipe.pid"


def start(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--service", action="store", default="yfs")
    parser.add_argument("--filesystem", action="store", default="nfs_share")
    parser.add_argument("--clustered", action="store_true", default=False)
    parser.add_argument("--verbose", action="store_true", default=False)
    parser.add_argument("--restart-interval", action="store", default=None)
    parser.add_argument("--restart-flag", action="store", default=None)
    args = parser.parse_args(argv)

    port_manager = common.network.PortManager()
    ganesha_port = port_manager.get_port()
    mon_port = port_manager.get_port()

    uid = str(uuid.uuid4())

    restart_interval = get_restart_interval(args.restart_interval)
    restart_flag = get_restart_flag(args.restart_flag, "ganesha-ready-" + uid)

    if args.service == "mem":
        nfs_ganesha = create_nfs_ganesha_mem(
            ganesha_port=ganesha_port,
            mon_port=mon_port,
            verbose=args.verbose,
            restart_interval=restart_interval,
            restart_flag=restart_flag)

    elif args.service == "vfs":
        nfs_ganesha = create_nfs_ganesha_vfs(
            ganesha_port=ganesha_port,
            mon_port=mon_port,
            filesystem=args.filesystem,
            verbose=args.verbose,
            restart_interval=restart_interval,
            restart_flag=restart_flag)

    elif args.service == "yfs":
        server_port = os.getenv("NFS_SERVER_PORT")

        client_path = common.binary_path(
            "cloud/filestore/client/filestore-client")

        client = NfsCliClient(
            binary_path=client_path,
            port=server_port,
            verbose=args.verbose,
            cwd=common.output_path())

        client.create(
            fs=args.filesystem,
            cloud="test_cloud",
            folder="test_folder",
            blk_size=BLOCK_SIZE,
            blk_count=BLOCKS_COUNT)

        nfs_ganesha = create_nfs_ganesha_yfs(
            ganesha_port=ganesha_port,
            mon_port=mon_port,
            server_port=server_port,
            filesystem=args.filesystem,
            clustered=args.clustered,
            verbose=args.verbose,
            restart_interval=restart_interval,
            restart_flag=restart_flag)

    else:
        raise RuntimeError("invalid service specified: " + args.service)

    nfs_ganesha.start()

    set_env("NFS_GANESHA_PORT", str(ganesha_port))

    with open(PID_FILE_NAME, "w") as f:
        f.write(str(nfs_ganesha.pid))

    wait_for_nfs_ganesha(nfs_ganesha, ganesha_port)


def stop(argv):
    if not os.path.exists(PID_FILE_NAME):
        return

    with open(PID_FILE_NAME) as f:
        pid = int(f.read())
        shutdown(pid)


if __name__ == "__main__":
    declare_recipe(start, stop)
