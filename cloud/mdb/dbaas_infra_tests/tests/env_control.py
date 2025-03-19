#!/usr/bin/env python3
"""
Manage test environment.

For config, see configuration.py.
"""

import argparse
import logging
import pickle

from tests import configuration
# These modules define stages to prepare the environment
from tests.helpers import (certs, compose, container_comm, docker, git, metadb, s3_template, salt_pki, ssh_pki,
                           templates, versioned_images)

SESSION_STATE_CONF = '.session_conf.sav'
STAGES = {
    'create': [
        # The order here is important: stages depend on previous` results.
        # e.g. you wont get much success building from docker-compose
        # unless you have base image in place.

        # Init salt pki
        salt_pki.init_pki,
        # Init ssh pki
        ssh_pki.init_pki,
        # Checkout the code into staging directory.
        git.checkout_code,
        # Create versioned images in stating
        versioned_images.create_staging,
        # Initialize PKI (generate CA, CRL, config, etc)
        certs.init_ssl,
        # Render configs using all available contexts
        templates.render_configs,
        # Update versioned images Dockerfiles in staging
        versioned_images.update_dockerfiles,
        # Prepare initial metadb data
        metadb.gen_initial_data,
        # Get container templates from S3
        s3_template.get_images,
        # Build docker images
        docker.build_images,
        # Create docker containers` network to enable
        # cross-container network communication.
        docker.prep_network,
        # Generate docker-compose.yml
        compose.create_config,
        # Dump certs for certificator
        certs.dump_ssl,
        # Start Arbiter to enable container-host crosscommunication.
        container_comm.start,
    ],
    'start': [compose.startup_containers],
    'start_arbiter': [container_comm.start_wait],
    'restart': [
        # Kill any remaining docker containers
        # that might have been created at runtime
        # (e.g. by DBM-API)
        docker.remove_containers,
        # Generate docker-compose.yml
        compose.create_config,
        # Calls `docker-compose up --force-recreate`
        compose.recreate_containers,
    ],
    'stop': [
        # Shutdown docker containers
        compose.shutdown_containers,
        # Kill any remaining docker containers
        docker.remove_containers,
        # Remove network bridges
        docker.shutdown_network,
        # Stop Arbiter
        container_comm.stop,
    ],
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

    return state


def create(state_file=None):
    """
    Create test environment.
    """
    state = _run_stage('create', state_file=state_file)

    _save_state({
        'salt_pki': state['salt_pki'],
        'ssh_pki': state['ssh_pki'],
        'config': state['config'],
        'arbiter': None,
        'ssl': None,
    },
                path=state_file)

    return state


def start(state=None, state_file=None):
    """
    Restart test environment runtime.
    """
    _run_stage('start', state=state, state_file=state_file)


def start_arbiter(state=None, state_file=None):
    """
    Start arbiter listener.
    """
    _run_stage('start_arbiter', state=state, state_file=state_file)


def restart(state=None, state_file=None):
    """
    Restart test environment runtime.
    """
    _run_stage('restart', state=state, state_file=state_file)


def stop(state=None, state_file=None):
    """
    Stop test environment runtime.
    """
    _run_stage('stop', state=state, state_file=state_file)


def _init_state(state_file=None):
    """
    Create state.
    If previous state file is found, restore it.
    If not, create new.
    """
    if state_file is None:
        state_file = SESSION_STATE_CONF
    state = {
        'config': None,
        'arbiter': None,
    }
    # Load previous state if found.
    try:
        with open(state_file, 'rb') as session_conf:
            return pickle.load(session_conf)
    except FileNotFoundError:
        # Clean slate: only need config for now, as
        # other stuff will be defined later.
        state['config'] = configuration.get()
    return state


def _save_state(conf, path=None):
    """
    Pickle state to disk.
    """
    if path is None:
        path = SESSION_STATE_CONF
    with open(path, 'wb') as session_conf:
        pickle.dump(conf, session_conf)


def parse_args(commands):
    """
    Parse command-line arguments.
    """
    arg = argparse.ArgumentParser(description="""
        DBaaS testing environment initializer script
        """)
    arg.add_argument(
        'command',
        choices=list(commands),
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
    arg.add_argument(
        '-v',
        '--verbose',
        default=False,
        action='store_true',
        help='print debug logs',
    )
    return arg.parse_args()


def cli_main():
    """
    CLI entry.
    """
    commands = {
        'create': create,
        'start': start,
        'start_arbiter': start_arbiter,
        'stop': stop,
    }

    args = parse_args(commands)
    logging.basicConfig(
        format='%(asctime)s [%(levelname)s]:\t%(message)s',
        level=logging.DEBUG if args.verbose else logging.INFO,
    )
    commands[args.command](state_file=args.state_file)


if __name__ == '__main__':
    cli_main()
