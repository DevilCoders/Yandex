#!/usr/bin/env python3

""" Module for cleaning stale docker images using only docker cli """

from datetime import datetime
import sys
import subprocess
from itertools import chain
import json
import os
import re
import logging
# TODO: add checks if tqdm is installed
# import tqdm


def call_cmd(*args):
    """ Just call command. Output goes to stdout. """
    cmd = list(chain.from_iterable(args))
    return subprocess.call(cmd)


def run_cmd(*args, out_format=None):
    """ Run command and return output, parsed and converted if needed """
    cmd = list(chain.from_iterable(args))
    # print(cmd)
    out = subprocess.check_output(cmd).decode("utf-8")
    if out_format is None:
        return out
    elif out_format == "json":
        return json.loads(out)
    elif out_format == "list":
        return list(filter(lambda x: x != "", out.split("\n")))
    else:
        raise NotImplementedError


def parse_date(str_date):
    """ Try several DateTime formats and match best one """
    date_formats = [
        {'r': r"(\d+\-\d+\-\d+T\d+:\d+:\d+)",
         's': r"%Y-%m-%dT%H:%M:%S"}]

    for dt_format in date_formats:
        dt_match = re.match(dt_format['r'], str_date)
        if dt_match is not None:
            return datetime.strptime(dt_match.group(0), dt_format['s'])
    return None


def main(use_sudo=False, verbose=None, progress=False):
    """ Check actual images and remove old """
    logger = logging.getLogger(sys.argv[0])
    docker_cmd = ['sudo', 'docker'] if use_sudo else ['docker']

    try:
        _ = run_cmd(docker_cmd, ["version"])
    except subprocess.CalledProcessError:
        logger.critical('Failed to get docker version using [%s]',
                        (" ".join(docker_cmd)))
        sys.exit(1)

    ps_a = ["ps", "-a", '--format', "{{.ID}}"]
    all_ids_list = run_cmd(docker_cmd, ps_a, out_format='list')

    image_stale_time = int(os.getenv("DOCKER_GC_IMAGE_STALE_TIME", 30*24*3600))
    container_stale_time = int(os.getenv("DOCKER_GC_CONTAINER_STALE_TIME", 7*24*3600))
    verbose = 'DOCKER_GC_VERBOSE' in os.environ if verbose is None else verbose

    ps_json = run_cmd(docker_cmd, ['inspect'], all_ids_list, out_format="json")
    used_images = set()
    stale_containers = []
    for container in ps_json:
        state = container['State']
        if state['Status'] == "running":
            used_images.add(container['Image'])
        else:
            finished_at = parse_date(state['FinishedAt'])
            if finished_at is None:
                logger.error("Failed to parse date [%s]", state['FinishedAt'])
                continue
            container_age = int((datetime.utcnow()-finished_at).total_seconds())
            if container_age < image_stale_time:
                used_images.add(container['Image'])
            if container_age > container_stale_time:
                stale_containers.append(container['Id'])

    logger.info("Images in use: " + " ".join(used_images))

    images_ids_list = run_cmd(docker_cmd, ['images', '--format', '{{.ID}}'],
                              out_format='list')
    images_json = run_cmd(docker_cmd, ['inspect'], images_ids_list,
                          out_format="json")
    images_to_remove = set()

    for image in images_json:
        if image['Id'] in used_images:
            logger.info("Keep %s %s", image['RepoTags'], image['Id'])
        else:
            images_to_remove.add(image['Id'])

    # TODO: propper detect if tqdm module is installed
    # if progress:
    #     i_stale_containers = tqdm.tqdm(stale_containers)
    #     i_images_to_remove = tqdm.tqdm(images_to_remove)
    # else:
    #     i_stale_containers = stale_containers
    #     i_images_to_remove = images_to_remove

    rm_errors = 0
    for container in stale_containers:
        logger.debug("rm %s", container)
        if call_cmd(docker_cmd, ["rm", container]) > 0:
            rm_errors += 1

    rmi_errors = 0
    for image in images_to_remove:
        logger.debug("rmi %s", image)
        if call_cmd(docker_cmd, ["rmi", image]) > 0:
            rmi_errors += 1

    logger.info("%d error(s) removing containers", rm_errors)
    logger.info("%d error(s) removing images", rmi_errors)
    if rm_errors > 0 or rmi_errors > 0:
        sys.exit(2)

if __name__ == "__main__":
    # pylint: disable=invalid-name
    filename, _ = os.path.splitext(sys.argv[0])
    logging.basicConfig(
        filename="/var/log/yandex/{}.log".format(os.path.basename(filename)),
        level=logging.DEBUG,
        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
    # TODO: use argparse to parse argv
    main(use_sudo=(len(sys.argv) > 1 and (sys.argv[1] in ["--sudo", "-s"])))
