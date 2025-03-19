"""
Base bundle binary for dbaas-worker
"""

import argparse
import os
import sys

from .cert_host import cert_host
from .create_service_accounts import create_service_accounts
from .conductor_migrate import conductor_migrate
from .dbaas_worker import dbaas_worker
from .fix_ext_dns import fix_ext_dns
from .fix_service_account import fix_service_account
from .force_resource_preset_change import force_resource_preset_change
from .free_dom0 import free_dom0
from .move_container import move_container
from .orphan_resources import orphan_resources
from .replace_rootfs import do_replace_rootfs
from .resetup_host import resetup_host
from .restore_deleted_cluster import restore_deleted_cluster
from .update_sgroups import update_sgroups
from .worker_shell import worker_shell
from .task_run import task_run

BASE_BIN = 'dbaas-worker'
BASE_FUN = dbaas_worker

LINK_MAP = {
    'cert-host': cert_host,
    'conductor-migrate': conductor_migrate,
    'create-service-accounts': create_service_accounts,
    'fix-ext-dns': fix_ext_dns,
    'fix-service-account': fix_service_account,
    'force-resource-preset-change': force_resource_preset_change,
    'free-dom0': free_dom0,
    'move-container': move_container,
    'orphan-resources': orphan_resources,
    'replace-rootfs': do_replace_rootfs,
    'resetup-host': resetup_host,
    'restore-deleted-cluster': restore_deleted_cluster,
    'update-sgroups': update_sgroups,
    'worker-shell': worker_shell,
    'task-run': task_run,
}


def create_symlinks():
    """
    Create symlinks beside base bin
    """
    dirname = os.path.dirname(sys.argv[0])
    target_path = os.path.join(dirname, BASE_BIN)
    for link in LINK_MAP:
        path = os.path.join(dirname, link)
        create = True
        if os.path.exists(path):
            if os.path.realpath(path) == target_path:
                create = False
            else:
                os.unlink(path)
        if create:
            print(f'Linking {path} to {target_path}')
            os.symlink(target_path, path)


def remove_symlinks():
    """
    Remove symlinks beside base bin
    """
    dirname = os.path.dirname(sys.argv[0])
    for link in LINK_MAP:
        path = os.path.join(dirname, link)
        if os.path.exists(path):
            print(f'Removing {path}')
            os.unlink(path)


def main():
    """
    Console entry-point
    """
    basename = os.path.basename(sys.argv[0])

    if basename == BASE_BIN:
        parser = argparse.ArgumentParser(add_help=False)
        group = parser.add_mutually_exclusive_group()
        group.add_argument('--create-symlinks', action='store_true')
        group.add_argument('--remove-symlinks', action='store_true')
        args, _ = parser.parse_known_args()

        if args.create_symlinks:
            return create_symlinks()
        if args.remove_symlinks:
            return remove_symlinks()

    if basename in LINK_MAP:
        return LINK_MAP[basename]()

    return BASE_FUN()
