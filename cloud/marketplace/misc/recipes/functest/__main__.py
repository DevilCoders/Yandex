import os
import socket
import sys
from time import sleep

import requests
import yatest.common
from library.python.testing.recipe import declare_recipe
from library.python.testing.recipe import set_env

from cloud.marketplace.misc.recipes.functest.config import Config

PREFIX = os.getenv("TEST_WORK_PATH", "/tmp")

API_STDERR = PREFIX + "/api.err"
API_STDOUT = PREFIX + "/api.out"
API_PID_FILE = PREFIX + "/api.pid"

AS_STDERR = PREFIX + "/as.err"
AS_STDOUT = PREFIX + "/as.out"
AS_PID_FILE = PREFIX + "/as.pid"

CLI_STDERR = PREFIX + "/cli_err"
CLI_STDOUT = PREFIX + "/cli_out"
CLI_ENV = {
    "YC_CONFIG_PATH": Config.API_CONFIG_PATH,
}

FUNC_TEST_UID = "bfb0b4curi3m01ij1m6c"


def find_free_ports(m):
    socks = []
    for i in range(m):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind(('', 0))
        socks.append(sock)

    ports = []
    for sock in socks:
        ports.append(sock.getsockname()[1])
        sock.close()

    return ports


def start(argv):
    set_env("YDB_CLIENT_VERSION", "2")
    ports = find_free_ports(3)  # API, AS_GRPC, AS_HTTP

    Config.generate(api_port=ports[0], as_grpc=ports[1], as_http=ports[2])

    yatest.common.execute([
        yatest.common.build_path("cloud/marketplace/cli/bin/yc_marketplace_cli"),
        "populate_db",
        "--devel",
        "--erase",
    ], env=CLI_ENV, stderr=CLI_STDERR, stdout=CLI_STDOUT)

    api_env = {
        "YC_CONFIG_PATH": Config.API_CONFIG_PATH,
        "YC_PORT": str(ports[0]),
        "MARKETPLACE_PUBLIC_IMG_FOLDER_ID": "noop",
    }
    api = yatest.common.execute([yatest.common.build_path("cloud/marketplace/api/wsgi/yc_marketplace")], wait=False,
                                env=api_env, stderr=sys.stderr, stdout=sys.stderr)

    with open(API_PID_FILE, "wt") as f:
        f.write(str(api.process.pid))

    as_mock = yatest.common.execute([yatest.common.build_path("cloud/iam/accessservice/mock/python/accessservice-mock"),
                                     "--port", str(ports[1]),
                                     "--control-server-port", str(ports[2])],
                                    wait=False, stderr=AS_STDERR, stdout=AS_STDOUT)
    with open(AS_PID_FILE, "wt") as f:
        f.write(str(as_mock.process.pid))

    sleep(5)

    as_data = {"user_account": {"id": FUNC_TEST_UID}}
    as_headers = {"Content-Type": "application/json"}
    set_env("FUNC_TEST_USER_ID", FUNC_TEST_UID)

    requests.put("http://localhost:{}/authenticate".format(ports[2]), json=as_data, headers=as_headers)
    requests.put("http://localhost:{}/authorize".format(ports[2]), json=as_data, headers=as_headers)

    set_env("TEST_CONFIGS_PATH", Config.TEST_CONFIG_PATH)


def stop(argv):
    with open(API_PID_FILE) as f:
        pid = int(f.read())
        os.kill(pid, 9)

    with open(AS_PID_FILE) as f:
        pid = int(f.read())
        os.kill(pid, 9)


if __name__ == "__main__":
    declare_recipe(start, stop)
