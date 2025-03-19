import argparse
import json
import os
import sys

from cloud.blockstore.pylibs.common import create_logger
from cloud.blockstore.pylibs.clients.sandbox import SandboxClient


class Error(Exception):
    pass


_YA_PACKAGE_TASK_TYPE = 'YA_PACKAGE_2'
_YA_PACKAGE_RESOURCE_TYPE = 'YA_PACKAGE'


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')

    parser.add_argument('--in-file', '--in',
                        help='path to file with packages to build (default: stdin)',
                        default=None)
    parser.add_argument('--out-file', '--out',
                        help='path to file with built packages versions (default: stdout)',
                        default=None)
    parser.add_argument('--timeout', type=int, default=3600)

    ya_package_group = parser.add_argument_group('ya package arguments')
    ya_package_group.add_argument('--arcadia-url',
                                  help='url to checkout arcadia from (default: arcadia:/arc/trunk/arcadia)',
                                  default='arcadia:/arc/trunk/arcadia')
    ya_package_group.add_argument('--build-type',
                                  help='packages build type (default: release)',
                                  default='release')
    ya_package_group.add_argument('--publish-to',
                                  help='debian repository to publish packages to (default: yandex-cloud)',
                                  default='yandex-cloud')
    ya_package_group.add_argument('--debian-distribution',
                                  help='debian distribution to publish packages to (default: unstable)',
                                  default='unstable')
    ya_package_group.add_argument('--sandbox-container',
                                  help='resource id of the lxc container image (default: none)')

    return parser.parse_args()


def _run_ya_package(args, logger):
    in_file = args.in_file if args.in_file is not None else '/dev/stdin'
    with open(in_file, 'r') as f:
        packages_to_build = f.read().splitlines()
    logger.debug(f'Packages list: {packages_to_build}')

    sandbox_oauth_token = os.getenv('SANDBOX_OAUTH_TOKEN')
    if sandbox_oauth_token is None:
        raise Error('no SANDBOX_OAUTH_TOKEN specified')
    sandbox = SandboxClient(token=sandbox_oauth_token, logger=logger)

    create_task_params = {
        'type': _YA_PACKAGE_TASK_TYPE,
        'owner': 'TEAMCITY',
        'priority': ('SERVICE', 'NORMAL'),
        'requirements': {
            'ram': 10 * 1024 ** 3
        },
        'kill_timeout': args.timeout + 600,
    }
    logger.debug(f'create_task_params: {create_task_params}')
    logger.info('Creating sandbox task')
    try:
        task = sandbox.create_task(create_task_params)
    except SandboxClient.Error as e:
        raise Error(f'failed to create sandbox task: {e}')
    logger.info(f'Created sandbox task <id={task.id}> <url={task.url}>')

    task_custom_fields = {
        'packages': ';'.join(packages_to_build),
        'use_aapi_fuse': True,
        'checkout_arcadia_from_url': args.arcadia_url,
        'build_type': args.build_type,
        'clear_build': False,
        'publish_to': args.publish_to,
        'debian_distribution': args.debian_distribution,
        'checkout_mode': 'auto',
        'build_system': 'ya_force',
        'ya_timeout': args.timeout,
        'sandbox_container': args.sandbox_container,
    }
    logger.debug(f'task_custom_fields: {task_custom_fields}')
    logger.info('Updating sandbox task custom fields')
    try:
        sandbox.update_task_custom_fields(task, task_custom_fields)
    except SandboxClient.Error as e:
        raise Error(f'failed to update sandbox task custom fields: {e}')

    logger.info('Starting sandbox task')
    try:
        sandbox.start_task(task)
    except SandboxClient.Error as e:
        raise Error(f'failed to start sandbox task: {e}')

    logger.info('Waiting until sandbox task finishes')
    try:
        ok = sandbox.wait_task(task)
    except SandboxClient.Error as e:
        raise Error(f'failed to wait sandbox task to finish: {e}')

    if not ok:
        raise Error(f'sandbox task failed <id={task.id}> <url={task.url}>')

    logger.info('Fetching sandbox task resources')
    task_resources = sandbox.fetch_resources(task)
    logger.debug(f'task_resources: {task_resources}')

    built_packages = []
    for task_resource in task_resources:
        if task_resource['type'] == _YA_PACKAGE_RESOURCE_TYPE:
            built_package = {
                'name': task_resource['attributes']['resource_name'],
                'version': task_resource['attributes']['resource_version'],
            }
            built_packages.append(built_package)
    logger.debug(f'built_packages: {built_packages}')

    out_file = args.out_file if args.out_file is not None else '/dev/stdout'
    with open(out_file, 'w') as f:
        json.dump(built_packages, f)


def main():
    args = _parse_args()
    logger = create_logger('yc-nbs-ci-run-ya-package', args)

    try:
        _run_ya_package(args, logger)
    except Error as e:
        logger.fatal(f'Failed to run ya package: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()
