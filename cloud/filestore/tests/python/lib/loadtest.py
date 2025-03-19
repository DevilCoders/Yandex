import logging

import yatest.common as common

logger = logging.getLogger(__name__)


def run_load_test(name, config, port):
    cmd = [
        common.binary_path(
            "cloud/filestore/tools/testing/loadtest/bin/filestore-loadtest"),
        "--port", str(port),
        "--tests-config", config,
    ]

    logger.info("launching load test: " + " ".join(cmd))
    return common.execute(cmd)
