#!/usr/bin/env python3
"""
Manage test environment.

For config, see configuration.py.
"""

import argparse
import os
import pickle
import sys

from tests import configuration

# These modules define stages to prepare the environment
from tests.helpers import compute_driver, deploy_api, hadoop_cluster, job_output, ssh_pki, utils, vault
from tests.logs import init_logger

SESSION_STATE_CONF = '.session_conf.sav'
STAGES = {
    'cache': [
        # We should recreate compute_driver from common-2if image, so delete and create it again
        compute_driver.rebuilded,
        # Rewrite salt configs
        compute_driver.rewrite_salt_configs,
        # Retrieve secrets from yandex vault
        vault.make_private_pillar,
        # Synchornize salt states and configs
        compute_driver.sync_salt,
        # Override pillar
        compute_driver.build_pillar_image,
        # Run two highstates
        compute_driver.highstate,
        # Disable all daemons before making cache
        compute_driver.disable_daemons,
        # Clean environment for instances with new fqdn
        compute_driver.cleanup,
        # Stop CP-VM
        compute_driver.stopped,
        # Create cache image
        compute_driver.cache_image,
    ],
    'start': [
        # Initialize ssh pki
        ssh_pki.init_pki,
        # Ensure that control plane VM exists
        compute_driver.exists,
        # Put ssh private key that will allow access to hadoop cluster nodes
        compute_driver.put_ssh_private_key,
        # Rewrite salt configs
        compute_driver.rewrite_salt_configs,
        # Retrieve secrets from yandex vault
        vault.make_private_pillar,
        # Synchornize salt and salt pillar
        compute_driver.sync_salt,
        # Synchornize dbaas_metadb code
        compute_driver.sync_metadb,
        # Override pillar
        compute_driver.build_pillar_regular,
        # Run two highstates
        compute_driver.highstate,
        # Build config for yc
        configuration.generate_yc_config,
        # Synchronize control-plane code for rewrite installed code from packages
        compute_driver.sync_code,
        # Restart daemons, for reloading code
        compute_driver.daemons_restart,
    ],
    'sync_salt_managed': [
        # Initialize ssh pki
        ssh_pki.init_pki,
        # Ensure that control plane VM exists
        compute_driver.exists,
        # Put ssh private key that will allow access to hadoop cluster nodes
        compute_driver.put_ssh_private_key,
        # Rewrite salt configs
        compute_driver.rewrite_salt_configs,
        # Retrieve secrets from yandex vault
        vault.make_private_pillar,
        # Get secrets for managed tests
        configuration.build_managed_secrets,
        # Synchornize salt and salt pillar
        compute_driver.sync_salt,
        # Synchornize salt and salt pillar for salt-master
        compute_driver.sync_salt_master,
        # Synchornize dbaas_metadb code
        compute_driver.sync_metadb,
        # Synchornize deploydb code
        compute_driver.sync_deploydb,
        # Override pillar
        compute_driver.build_pillar_managed,
    ],
    'start_managed': [
        # set config flag to know if it's managed mode
        configuration.set_managed_flag,
        # Initialize ssh pki
        ssh_pki.init_pki,
        # Ensure that control plane VM exists
        compute_driver.exists,
        # Put ssh private key that will allow access to hadoop cluster nodes
        compute_driver.put_ssh_private_key,
        # Rewrite salt configs
        compute_driver.rewrite_salt_configs,
        # Retrieve secrets from yandex vault
        vault.make_private_pillar,
        # Get secrets for managed tests
        configuration.build_managed_secrets,
        # Synchornize salt and salt pillar
        compute_driver.sync_salt,
        # Synchornize salt and salt pillar for salt-master
        compute_driver.sync_salt_master,
        # Synchornize dbaas_metadb code
        compute_driver.sync_metadb,
        # Synchornize deploydb code
        compute_driver.sync_deploydb,
        # Override pillar
        compute_driver.build_pillar_managed,
        # Run two highstates
        compute_driver.highstate,
        # Build config for yc
        configuration.generate_yc_config,
        # Put infratest CA to staging/infratest.pem
        configuration.put_infratest_ca,
        # Synchronize control-plane code for rewrite installed code from packages
        compute_driver.sync_code,
        # Restart daemons, for reloading code
        compute_driver.daemons_restart_managed,
        # Create deploy-api master and deploy group
        deploy_api.register_master,
    ],
    'sync_code': [
        # Update ssh key pair on changes
        compute_driver.put_ssh_private_key,
        # Synchronize control-plane code for rewrite installed code from packages
        compute_driver.sync_code,
        # Restart daemons, for reloading code
        compute_driver.daemons_restart,
    ],
    'fast_start': [
        # Initialize ssh pki
        ssh_pki.init_pki,
    ],
    'stop': [
        # Shutdown driver
        compute_driver.stopped,
    ],
    'clean': [
        # Remove hosts and clean database
        compute_driver.cleanup_environment_step,
        # Shutdown driver
        compute_driver.absent,
    ],
    'watch_job_output': [job_output.watch],
    'ssh_to_master': [hadoop_cluster.ssh_to_master],
    'gc_infratest_resources': [
        compute_driver.gc_infratest_vms,
        compute_driver.gc_infratest_snapshots,
        compute_driver.gc_infratest_instance_groups,
        compute_driver.gc_infratest_s3,
        compute_driver.gc_infratest_security_groups,
    ],
    'list_infratest_vms': [compute_driver.list_infratest_vms],
    'sync_salt': [compute_driver.sync_salt],
}


def _run_stage(event, state=None, state_file=None):
    """
    Run stage steps.
    """
    assert event in STAGES, event + ' not implemented'

    if not state:
        state = _init_state(state_file)

    for step in STAGES[event]:
        step(state=state, conf=state['config'])

    _save_state(
        {
            'ssh_pki': state.get('ssh_pki'),
            'ssl': None,
            'persistent': state.get('persistent', {}),
        },
        path=state_file,
    )

    return state


def stop(state=None, state_file=None):
    """
    Stop compute test environment.
    """
    _run_stage('stop', state=state, state_file=state_file)


def start(state_file=None):
    """
    Start compute test environment.
    """
    state = _init_state(state_file)

    disable_init = state['config'].get('disable_init', False)
    if os.environ.get('DISABLE_INIT') is not None:
        disable_init = str(os.environ.get('DISABLE_INIT')).lower() == 'true'

    if disable_init:
        state = _run_stage('fast_start', state=state, state_file=state_file)
    else:
        state = _run_stage('start', state=state, state_file=state_file)

    return state


def _init_state(state_file=None):
    """
    Create state.
    If previous state file is found, restore it.
    If not, create new.
    """
    if state_file is None:
        state_file = SESSION_STATE_CONF
    # Load previous state if found.
    try:
        with open(state_file, 'rb') as session_conf:
            state = pickle.load(session_conf)
    except FileNotFoundError:
        state = {}

    state['config'] = configuration.build(state_file)

    return state


def _save_state(conf, path=None):
    """
    Pickle state to disk.
    """
    if path is None:
        path = SESSION_STATE_CONF
    try:
        with open(path, 'rb') as session_conf:
            state = pickle.load(session_conf)
    except FileNotFoundError:
        state = {}
    state = utils.merge(state, conf)
    with open(path, 'wb') as session_conf:
        pickle.dump(state, session_conf)


def parse_args(commands):
    """
    Parse command-line arguments.
    """
    arg = argparse.ArgumentParser(
        description="""
        DBaaS testing environment initializer script
        """
    )
    arg.add_argument(
        'command',
        choices=commands,
        help='command to perform',
    )
    arg.add_argument(
        '-s',
        '--state-file',
        dest='state_file',
        type=str,
        metavar='<path>',
        default=SESSION_STATE_CONF,
        help='path to state file (pickle dump)',
    )
    return arg.parse_args()


def cli_main():
    """
    CLI entry.
    """
    module = sys.modules[__name__]
    module_methods = [method_name for method_name in dir(module) if callable(getattr(module, method_name))]

    init_logger()

    commands = STAGES.keys()
    args = parse_args(commands)
    command = args.command
    if command in module_methods:
        getattr(module, command)(state_file=args.state_file)
    else:
        _run_stage(command, state_file=args.state_file)


if __name__ == '__main__':
    cli_main()
