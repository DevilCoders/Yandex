import argparse
import os
import sys

from cloud.blockstore.pylibs.common import create_logger
from cloud.blockstore.pylibs.clients.sandbox import SandboxClient

NETCONFIG = '''
#
# The network configuration file. This file is currently only used in
# conjunction with the TI-RPC code in the libtirpc library.
#
# Entries consist of:
#
#       <network_id> <semantics> <flags> <protofamily> <protoname> \
#               <device> <nametoaddr_libs>
#
# The <device> and <nametoaddr_libs> fields are always empty in this
# implementation.
#
udp        tpi_clts      v     inet     udp     -       -
tcp        tpi_cots_ord  v     inet     tcp     -       -
udp6       tpi_clts      v     inet6    udp     -       -
tcp6       tpi_cots_ord  v     inet6    tcp     -       -
rawip      tpi_raw       -     inet      -      -       -
local      tpi_cots_ord  -     loopback  -      -       -
unix       tpi_cots_ord  -     loopback  -      -       -
'''

CUSTOM_SCRIPT = [
    'cat >/etc/netconfig <<EOL' + NETCONFIG + 'EOL',
]

CUSTOM_PACKAGES = [
    'rpcbind',
]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true')
    return parser.parse_args()


def run_task(args, logger):
    sandbox_oauth_token = os.getenv('SANDBOX_OAUTH_TOKEN')
    if sandbox_oauth_token is None:
        raise RuntimeError('no SANDBOX_OAUTH_TOKEN specified')

    sandbox = SandboxClient(token=sandbox_oauth_token, logger=logger)

    create_task_params = {
        'type': 'SANDBOX_LXC_IMAGE',
        'owner': 'TEAMCITY',
        'priority': ('SERVICE', 'NORMAL'),
    }

    logger.debug(f'create_task_params: {create_task_params}')
    logger.info('Creating sandbox task')
    try:
        task = sandbox.create_task(create_task_params)
    except SandboxClient.Error as e:
        raise RuntimeError(f'failed to create sandbox task: {e}')
    logger.info(f'Created sandbox task <url={task.url}>')

    task_custom_fields = {
        'install_common': True,
        'custom_image': True,
        'custom_script': '\n'.join(CUSTOM_SCRIPT),
        'custom_packages': ','.join(CUSTOM_PACKAGES),
    }

    logger.debug(f'task_custom_fields: {task_custom_fields}')
    logger.info('Updating sandbox task custom fields')
    try:
        sandbox.update_task_custom_fields(task, task_custom_fields)
    except SandboxClient.Error as e:
        raise RuntimeError(f'failed to update sandbox task custom fields: {e}')

    logger.info('Starting sandbox task')
    try:
        sandbox.start_task(task)
    except SandboxClient.Error as e:
        raise RuntimeError(f'failed to start sandbox task: {e}')

    logger.info('Waiting until sandbox task finishes')
    try:
        ok = sandbox.wait_task(task)
    except SandboxClient.Error as e:
        raise RuntimeError(f'failed to wait sandbox task to finish: {e}')

    if not ok:
        raise RuntimeError(
            f'sandbox task failed <id={task.id}> <url={task.url}>')

    logger.info(f'Sandbox task {task.id} completed')


def main():
    args = parse_args()
    logger = create_logger('build-test-container', args)
    try:
        run_task(args, logger)
    except RuntimeError as e:
        logger.fatal(f'Failed to run build task: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()
