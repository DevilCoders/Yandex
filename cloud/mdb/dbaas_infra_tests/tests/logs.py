"""
Logs save handler
"""

import io
import json
import os
import re
import tarfile
from traceback import print_exc

from retrying import retry

from .helpers import database, docker
from .helpers.docker import get_containers
from .helpers.utils import context_to_dict


def drop_symlinks(path):
    """
    Recursively remove symlinks in path
    """
    for item in os.listdir(path):
        item_path = os.path.join(path, item)
        if os.path.islink(item_path):
            os.unlink(item_path)
        elif os.path.isdir(item_path):
            drop_symlinks(item_path)


def save_container_dir(container, container_dir, local_dir, ignored=None):
    """
    Save docker directory.
    """
    archive, _ = container.get_archive(container_dir)
    raw_archive = io.BytesIO(b''.join(archive))
    tar = tarfile.open(mode='r', fileobj=raw_archive)

    members = tar.getmembers()
    for pattern in ignored or []:
        members = [m for m in members if not re.match(pattern, m.name)]

    tar.extractall(path=local_dir, members=members)


def save_journald_logs(container, local_dir):
    """
    Save journald logs in text format
    """
    exit_code, ret_bytes = container.exec_run('journalctl --no-pager')
    if exit_code != 0:
        # executable file not found in $PATH": unknown
        if exit_code == 126:
            return
        print('Unable to dump journald logs')
        print(ret_bytes)
    with open(os.path.join(local_dir, 'journald.log'), 'wb') as out:
        out.write(ret_bytes)


def save_container_logs(container, logs_dir):
    """
    Save docker logs and /var/log dir
    """
    base = os.path.join(logs_dir, container.name)
    os.makedirs(base, exist_ok=True)
    with open(os.path.join(base, 'docker.log'), 'wb') as out:
        out.write(container.logs(stdout=True, stderr=True, timestamps=True))

    save_container_dir(container, '/etc', base, ignored=['etc/ssl'])
    save_container_dir(container, '/var/log', base)
    save_journald_logs(container, base)


def save_context(context, logs_dir):
    """
    Save behave context
    """
    with open(os.path.join(logs_dir, 'context.json'), 'w') as out:
        json.dump(context_to_dict(context), out, default=repr, indent=4)


@retry(wait_fixed=250, stop_max_attempt_number=20)
def save_logs(context):
    """
    Save logs and support materials
    """
    logs_dir = os.path.join(context.conf['staging_dir'], 'logs', context.scenario.name)
    os.makedirs(logs_dir, exist_ok=True)
    drop_symlinks(logs_dir)

    save_context(context, logs_dir)

    for srv in ('metadb', 'deploy_db', 'mlockdb'):
        srv_logdir = os.path.join(logs_dir, srv)
        try:
            database.pg_dump_csv(context, srv, srv_logdir)
        except Exception:
            print('Unable to save database dump for {0}'.format(srv))
            print_exc()

    try:
        s3_container = docker.get_container(context, 'minio01')
        s3_logdir = os.path.join(logs_dir, 's3')
        save_container_dir(s3_container, '/export', s3_logdir)
    except Exception:
        print('Unable to save s3 dump')
        print_exc()

    for container in get_containers(context.conf):
        try:
            save_container_logs(container, logs_dir)
        except Exception:
            print('Unable to save logs for container {0}'.format(container.name))
            print_exc()

    drop_symlinks(logs_dir)
