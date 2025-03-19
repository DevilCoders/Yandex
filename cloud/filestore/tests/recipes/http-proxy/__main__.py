import os
import signal

import yatest.common as common

from library.python.testing.recipe import declare_recipe, set_env

from cloud.filestore.tests.python.lib.http_proxy \
    import create_nfs_http_proxy, wait_for_nfs_server_proxy


PROXY_PID_FILE_NAME = "http_proxy_nfs_vhost_recipe.pid"


def start(argv):
    vhost_port = os.getenv("NFS_VHOST_PORT")

    port_manager = common.network.PortManager()
    proxy_port = port_manager.get_port()
    proxy = create_nfs_http_proxy(proxy_port, vhost_port)
    proxy.start()

    set_env("NFS_HTTP_PROXY_PORT", str(proxy_port))

    with open(PROXY_PID_FILE_NAME, "w") as f:
        f.write(str(proxy.pid))

    wait_for_nfs_server_proxy(proxy_port)


def stop(argv):
    with open(PROXY_PID_FILE_NAME) as f:
        pid = int(f.read())
        os.kill(pid, signal.SIGTERM)


if __name__ == "__main__":
    declare_recipe(start, stop)
