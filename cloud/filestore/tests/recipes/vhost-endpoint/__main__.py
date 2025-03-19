import argparse
import logging
import os
import uuid

import yatest.common as common

from library.python.testing.recipe import declare_recipe, set_env

from cloud.filestore.tests.python.lib.client import NfsCliClient

import cloud.filestore.public.sdk.python.protos as protos


KEYRING_FILE_NAME = "vhost-endpoint-keyring-name.txt"
logger = logging.getLogger(__name__)


def start(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--filesystem", action="store", default="nfs_share")
    parser.add_argument("--socket-path", action="store", default="/tmp")
    parser.add_argument("--socket-prefix", action="store", default="test.vhost")
    parser.add_argument("--verbose", action="store_true", default=False)
    args = parser.parse_args(argv)

    port = os.getenv("NFS_SERVER_PORT")
    vhost_port = os.getenv("NFS_VHOST_PORT")

    # Create filestore
    client_path = common.binary_path(
        "cloud/filestore/client/filestore-client")

    client = NfsCliClient(
        client_path, port,
        vhost_port=vhost_port,
        verbose=args.verbose,
        cwd=common.output_path())

    client.create(
        args.filesystem,
        "test_cloud",
        "test_folder")

    _uid = str(uuid.uuid4())

    socket = os.path.join(
        args.socket_path,
        args.socket_prefix + "." + _uid)
    socket = os.path.abspath(socket)

    endpoint_storage_dir = os.getenv("NFS_VHOST_ENDPOINT_STORAGE_DIR", None)
    if endpoint_storage_dir is not None:
        # create endpoint and put it into endpoint storage
        start_endpoint = protos.TStartEndpointRequest(
            Endpoint=protos.TEndpointConfig(
                FileSystemId=args.filesystem,
                SocketPath=socket,
            )
        )

        keyring_id = 42
        with open(endpoint_storage_dir + "/" + str(keyring_id), 'wb') as f:
            f.write(start_endpoint.SerializeToString())

        client.kick_endpoint(keyring_id)
    else:
        client.start_endpoint(args.filesystem, socket)

    set_env("NFS_VHOST_SOCKET", socket)


def stop(argv):
    vhost_port = os.getenv("NFS_VHOST_PORT")
    socket = os.getenv("NFS_VHOST_SOCKET")

    if not vhost_port or not socket or not os.path.exists(socket):
        return

    client_path = common.binary_path(
        "cloud/filestore/client/filestore-client")

    client = NfsCliClient(
        client_path, port=None,
        vhost_port=vhost_port,
        verbose=True,
        cwd=common.output_path())
    client.stop_endpoint(socket)


if __name__ == "__main__":
    declare_recipe(start, stop)
