#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import json
import tempfile
import logging
from logging.handlers import RotatingFileHandler

from retrying import retry


def _get_logger():
    """
    Initialize logger
    """
    logger = logging.getLogger("dataproc-init-actions")
    logger.setLevel(logging.DEBUG)
    log_handler = RotatingFileHandler(
        "/var/log/yandex/dataproc-init-actions.log", maxBytes=10 * 1024 * 1024, backupCount=5
    )
    log_handler.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s]:\t%(message)s"))
    logger.addHandler(log_handler)

    return logger


logger = _get_logger()


def _get_metadata(path):
    """
    Get metadata from pillar
    """
    command_args = ["salt-call", "--out", "json", "--local"]
    command_args += path.split()

    process = subprocess.run(command_args, capture_output=True)
    if process.returncode != 0:
        logger.error("Command %s failed with error: %s", process.args, process.stderr.decode("utf-8").strip())
        raise Exception("salt-call command execution failed.")

    result = None

    try:
        result = json.loads(process.stdout)["local"]
    except:
        logger.exception(
            "Unable to parse values from pillar. Given object to parse: %s", process.stdout.decode("utf-8").strip()
        )

    return result


def _get_initialization_actions():
    """
    Get initialization_actions from pillar
    """
    command_args = "salt-call --out json --local pillar.get data:initialization_actions"
    process = subprocess.run(command_args, capture_output=True, shell=True)
    if process.returncode != 0:
        logger.error("Command %s failed with error: %s", process.args, process.stderr.decode("utf-8").strip())
        raise Exception("salt-call command execution failed.")

    init_actions_list = []

    try:
        init_actions_list = json.loads(process.stdout)["local"]
    except:
        logger.exception(
            "Unable to parse initialization_actions from pillar. Given object to parse: %s",
            process.stdout.decode("utf-8").strip(),
        )

    return init_actions_list


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=8)
def _download(command_args):
    subprocess.run(command_args, timeout=30, check=True)


def get_environment():
    env_vars = {
        "CLUSTER_ID": _get_metadata("ydputils.get_cid"),
        "ROLE": " ".join(_get_metadata("ydputils.roles")),  #  list of roles for instance
        "MIN_WORKER_COUNT": str(_get_metadata("ydputils.get_min_worker_count")),
        "MAX_WORKER_COUNT": str(_get_metadata("ydputils.get_max_worker_count")),
    }
    bucket = _get_metadata("ydputils.get_s3_bucket")
    if bucket:
        env_vars["S3_BUCKET"] = bucket
    services = " ".join(_get_metadata("ydputils.get_cluster_services"))
    if services:
        env_vars["CLUSTER_SERVICES"] = services
    path = os.getenv("PATH")
    env_vars['PATH'] = f"/opt/yandex-cloud/bin:{path}"
    return env_vars


def _download_and_execute(init_actions_list):
    if not init_actions_list:
        logger.info("No initialization_actions provided.")
        return
    env = get_environment()
    with tempfile.TemporaryDirectory() as temp_dir:
        for init_action in init_actions_list:
            filepath = init_action["uri"]
            filename = os.path.basename(filepath)
            tmpfile = os.path.join(temp_dir, filename)
            args = init_action.get("args", [])
            timeout = init_action.get("timeout", 10 * 60)
            logger.info("Starting download of %s", filepath)
            try:
                _download(["hadoop", "fs", "-copyToLocal", "-f", filepath, tmpfile])
                logger.info("Successful download of %s", filepath)
            except:
                logger.exception("All attempts of download have failed.")
                return
            _execute_initialization_action(tmpfile, args, timeout, env)


def _execute_initialization_action(userfile, args, timeout, env):
    logger.info("Starting execution of %s", userfile)
    os.chmod(userfile, 0o700)

    process = subprocess.run([userfile] + args, stderr=subprocess.PIPE, timeout=timeout, env=env)

    if process.returncode != 0:
        logger.error(
            "Execution of %s failed with error: %s Exit-code: %d",
            userfile,
            process.stderr.decode("utf-8").strip(),
            process.returncode,
        )
        raise Exception("Initialization action failed.")


def main():
    init_actions_list = _get_initialization_actions()
    _download_and_execute(init_actions_list)


if __name__ == "__main__":
    main()
