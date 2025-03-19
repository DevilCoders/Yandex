"""
helpers for ssh keys
"""

import os
import tempfile
import logging
import subprocess

from typing import Sequence, Union
from logging import Logger

from helpers import vault

LOG = logging.getLogger('ssh')


def load_key(ctx):
    """
    Load ssh public key
    """
    conf = ctx.conf
    path = conf.get('ssh_public_key_file_path')
    # Use ssh pair from ssh-agent and file path
    if path:
        with open(os.path.expanduser(path)) as fh:
            ctx.state['ssh_public_key'] = fh.read()
        return
    # Otherwise, use ssh pair from vault
    secrets = vault.get_version(ctx, conf['ssh_key_yav_id'], packed_value=False)['value']
    for secret in secrets:
        if secret.get('key') == 'public':
            ctx.state['ssh_public_key'] = secret['value']
        if secret.get('key') == 'private':
            with tempfile.NamedTemporaryFile(mode='w', delete=False) as fp:
                fp.write(secret['value'] + '\n')
                ctx.state['ssh_private_key'] = fp.name


def clean_key(ctx):
    """
    Remove temporary file with ssh private key
    """
    key = ctx.state.get('ssh_private_key')
    if key:
        os.remove(key)


def execute(host: str,
            command: str,
            user: str = 'root',
            options: Sequence = ('-o', "StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"),
            stdout: int = subprocess.PIPE,
            stderr: int = subprocess.PIPE,
            timeout: Union[int, None] = 300,
            logger: Logger = LOG) -> (int, str, str):
    """
    Execute command on remote host
    Return Tuple of (return_code, stdout, stderr)
    """
    logger.debug(f'Executing command {command} on {host}')
    args = ['ssh'] + list(options) + [f'{user}@{host}', command]
    ret = subprocess.run(args, stdout=stdout, stderr=stderr, timeout=timeout)
    return ret.returncode, ret.stdout, ret.stderr
