import argparse
import os
import requests
import sys

from cloud.blockstore.pylibs.common import create_logger
from cloud.blockstore.pylibs.clients.sandbox import SandboxClient

from sandbox.common.types.resource import State


_YA_MAKE_TASK_TYPE = 'YA_MAKE'
_YA_MAKE_TASK_LOGS_TYPE = 'TASK_LOGS'


class Error(Exception):
    pass


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')

    parser.add_argument('--test-targets-file',
                        help='path to file with test targets (default: stdin)',
                        default=None)

    parser.add_argument('--timeout',
                        type=int,
                        help='Ya make timeout (+10 minutes task timeout)',
                        default=3600)
    parser.add_argument('--ram',
                        type=int,
                        help='Host required ram in GiB',
                        default=10)
    parser.add_argument('--cores',
                        type=int,
                        help='Host required cores',
                        default=12)
    parser.add_argument('--threads',
                        type=int,
                        help='Test thread count',
                        default=32)
    parser.add_argument('--disk_space',
                        type=int,
                        help='Host required disk space in GiB',
                        default=40)

    ya_make_group = parser.add_argument_group('ya make arguments')
    ya_make_group.add_argument('--arcadia-url',
                               help='arcadia url (default: arcadia:/arc/trunk/arcadia)',
                               default='arcadia:/arc/trunk/arcadia')
    ya_make_group.add_argument('--build-type',
                               help='build type (default: release)',
                               default='release')
    ya_make_group.add_argument('--sanitize',
                               help='build with specified sanitizer (default: None)',
                               default=None)
    ya_make_group.add_argument('--definition-flags',
                               help='definition flags (default: -DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY)',
                               default='-DSKIP_JUNK -DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY')
    ya_make_group.add_argument('--container-resource',
                               help='run task in the specified container',
                               default=None)
    ya_make_group.add_argument('--env-vars',
                               help="environment variables for sandbox task (e.g. VAR1=val1 VAR2='v a l 2')",
                               default='')
    ya_make_group.add_argument('--privileged',
                               help='ask to run tests as root',
                               action="store_true",
                               default=False)
    ya_make_group.add_argument('--run-tagged-tests-on-sandbox',
                               help='run tests tagged with ya:force_sandbox using subtask for each test',
                               action="store_true",
                               default=False)
    ya_make_group.add_argument('--use-arc',
                               help='use arc instead of aapi',
                               action="store_true",
                               default=False)
    ya_make_group.add_argument('--disable-test-timeout',
                               help='turn off timeout for tests',
                               action="store_true",
                               default=False)

    return parser.parse_args()


def _run_ya_package(args, logger):
    test_targets_file = args.test_targets_file if args.test_targets_file is not None else '/dev/stdin'
    with open(test_targets_file, 'r') as f:
        test_targets = f.read().splitlines()
    logger.debug(f'Test targets: {test_targets}')

    sandbox_oauth_token = os.getenv('SANDBOX_OAUTH_TOKEN')
    if sandbox_oauth_token is None:
        raise Error('no SANDBOX_OAUTH_TOKEN specified')
    sandbox = SandboxClient(token=sandbox_oauth_token, logger=logger)

    create_task_params = {
        'type': _YA_MAKE_TASK_TYPE,
        'owner': 'TEAMCITY',
        'priority': ('SERVICE', 'NORMAL'),
        'requirements': {
            'container_resource': args.container_resource,
            'cores': args.cores,
            'ram': args.ram * 1024 ** 3,
            'disk_space': args.disk_space * 1024 ** 3,
        },
        'kill_timeout': args.timeout + 600,
    }
    logger.debug(f'create_task_params: {create_task_params}')
    logger.info('Creating sandbox task')
    try:
        task = sandbox.create_task(create_task_params)
    except SandboxClient.Error as e:
        raise Error(f'failed to create sandbox task: {e}')
    logger.info(f'Created sandbox task <url={task.url}>')

    task_custom_fields = {
        'targets': ';'.join(test_targets),
        'disable_test_timeout': args.disable_test_timeout,
        'use_aapi_fuse': True,
        'use_arc_instead_of_aapi': args.use_arc,
        'checkout_arcadia_from_url': args.arcadia_url,
        'build_type': args.build_type,
        'sanitize': args.sanitize,
        'build_system': 'ya_force',
        'definition_flags': args.definition_flags,
        'keep_on': True,
        'test': True,
        'test_threads': args.threads,
        'junit_report': True,
        'ya_timeout': args.timeout,
        'privileged': args.privileged,
        'run_tagged_tests_on_sandbox': args.run_tagged_tests_on_sandbox,
        'env_vars': args.env_vars,
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

    logger.info('Fetching sandbox task resources')
    task_resources = sandbox.fetch_resources(task)
    logger.debug(f'task_resources: {task_resources}')

    junit_report_url = None

    for task_resource in task_resources:
        if task_resource["type"] != _YA_MAKE_TASK_LOGS_TYPE:
            continue
        if task_resource["state"] != State.READY:
            continue
        report_path = os.path.join(
            task_resource['http']['proxy'],
            'junit_report.xml')
        if requests.head(report_path).status_code != requests.codes.ok:
            continue
        junit_report_url = report_path

    if junit_report_url is None:
        logger.warning('Failed to find junit XML report in task resources')
    else:
        r = requests.get(junit_report_url)
        with open('junit_report.xml', 'wb') as f:
            f.write(r.content)

    if not ok:
        raise Error(f'sandbox task failed <id={task.id}> <url={task.url}>')


def main():
    args = _parse_args()
    logger = create_logger('yc-nbs-ci-run-ya-make', args)

    try:
        _run_ya_package(args, logger)
    except Error as e:
        logger.fatal(f'Failed to run ya make: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()
