import logging

import requests

import yatest.common
import yatest.common.network

from library.python.testing import recipe
from library.recipes import common as recipes_common


STUB_ARCADIA_PATH = "contrib/python/moto/bin/moto_server"
S3MDS_PID_FILE = "s3mds.pid"

logger = logging.getLogger("maps.pylibs.recipes.s3mds")


def start(argv):
    pm = yatest.common.network.PortManager()
    stub_port = str(pm.get_port())

    logger.info(f"Will start S3MDS stub on localhost:{stub_port}")
    recipe.set_env("S3MDS_PORT", stub_port)

    command = [
        yatest.common.binary_path(STUB_ARCADIA_PATH),
        "s3",
        "--port", stub_port
    ]

    def is_stub_ready():
        try:
            response = requests.get(f"http://localhost:{stub_port}")
            response.raise_for_status()
            return True
        except requests.RequestException as err:
            logger.debug(err)
            return False

    recipes_common.start_daemon(
        command=command,
        environment=None,
        is_alive_check=is_stub_ready,
        pid_file_name=S3MDS_PID_FILE
    )


def stop(argv):
    with open(S3MDS_PID_FILE) as f:
        pid = f.read()

    logger.info(f"Stopping s3mds stub (pid={pid})")

    if not recipes_common.stop_daemon(pid):
        logger.warning(f"Process with pid={pid} is already dead")


def main():
    recipe.declare_recipe(start, stop)
