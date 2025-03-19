"""
General purpose stuff, like dict merging or str.format() template filler.
"""
import collections.abc
import copy
import logging
import os
import re
from retrying import retry
import subprocess
import tempfile
from functools import wraps
from typing import List, Sequence, Union

import sshtunnel
import yaml
from jinja2 import Template

ALL_CA_LOCAL_PATH = '../salt/salt/components/common/conf/allCAs.pem'
PREPROD_CA_LOCAL_PATH = '../salt/salt/components/dbaas-internal-api/conf/YandexCLCA.pem'


class SSHError(Exception):
    """
    SSH Exception
    """


class DNSError(SSHError):
    """
    SSH command failed because of dns resolution failure
    """


class RsyncError(SSHError):
    """
    Rsync Exception
    """


def fix_rel_path(path):
    """
    Fix rel path if it points to directory upper in hier
    """
    if path.startswith('..'):
        return os.path.abspath(path)
    return path


def merge(original, update):
    """
    Recursively merge update dict into original.
    """
    for key in update:
        recurse_conditions = [
            # Does update have same key?
            key in original,
            # Do both the update and original have dicts at this key?
            isinstance(original.get(key), dict),
            isinstance(update.get(key), collections.abc.Mapping),
        ]
        if all(recurse_conditions):
            merge(original[key], update[key])
        else:
            original[key] = update[key]
    return original


def format_object(obj, **replacements):
    """
    Replace format placeholders with actual values
    """
    if isinstance(obj, str):
        obj = obj.format(**replacements)
    elif isinstance(obj, collections.abc.Mapping):
        for key, value in obj.items():
            obj[key] = format_object(value, **replacements)
    elif isinstance(obj, collections.abc.Iterable):
        for idx, val in enumerate(obj):
            obj[idx] = format_object(val, **replacements)
    return obj


def split(string, separator=',', strip=True):
    """
    Split string on tokens using the separator, optionally strip them and
    return as list.
    """
    return [v.strip() if strip else v for v in string.split(separator)]


def get_env_without_python_entry_point():
    env = copy.deepcopy(os.environ)
    for env_var in ('Y_PYTHON_ENTRY_POINT', 'Y_PYTHON_SOURCE_ROOT'):
        if env_var in env:
            del env[env_var]
    return env


def env_stage(event, fail=False):
    """
    Nicely logs env stage
    """

    def decorator(fun):
        @wraps(fun)
        def wrapper(*args, **kwargs):
            stage_name = '{mod}.{fun}'.format(
                mod=fun.__module__,
                fun=fun.__name__,
            )
            logging.info('initiating %s stage %s', event, stage_name)
            try:
                return fun(*args, **kwargs)
            except Exception as exc:
                logging.error('%s failed: %s', stage_name, exc)
                if fail:
                    raise

        return wrapper

    return decorator


def context_to_dict(context):
    """
    Convert behave context to dict representation.
    """
    result = {}
    for frame in context._stack:  # pylint: disable=protected-access
        for key, value in frame.items():
            if key not in result:
                result[key] = value

    return result


def render_template(template, template_context):
    """
    Render jinja template
    """
    return Template(template).render(**template_context)


def camelcase_to_underscore(obj):
    """
    Converts camelcase to underscore
    """
    return re.compile(r'[A-Z]').sub(lambda m: '_' + m.group().lower(), obj)


def convert_prefixed_upperscore(obj):
    """
    Converts `WAL_LEVEL_LOGICAL` to `logical`
    """
    if not isinstance(obj, str):
        return obj
    reg = re.compile(r'[A-Z_]+')
    if reg.match(obj):
        return reg.sub(lambda m: m.group().split('_')[-1].lower(), obj)
    return obj


def dict_convert_keys(obj):
    """
    Recursively converts keys of `obj` from camelcase to underscore
    """
    new_obj = dict()
    for key, value in obj.items():
        if isinstance(value, dict):
            dict_convert_keys(value)
        else:
            # Convert key to underscore, value 'WAL_LEVEL_LOGICAL' to 'logical'
            new_obj[camelcase_to_underscore(key)] = convert_prefixed_upperscore(value)
    return new_obj


def open_tunnel(context, hostname, port, ssh_username='ubuntu', jump_host=None):
    hostname_for_internal_interface = hostname.replace('mdb.cloud-preprod', 'db')
    tunnel = sshtunnel.open_tunnel(
        jump_host or hostname_for_internal_interface,
        ssh_username=ssh_username,
        ssh_port=22,
        remote_bind_address=(hostname_for_internal_interface, port),
        ssh_pkey=context.conf['ssh']['pkey_path'],
        ssh_private_key_password=context.conf['ssh']['pkey_password'],
    )
    tunnel.start()
    logging.info(f'Tunnel opened to {hostname_for_internal_interface}')
    context.tunnels_pool[hostname_for_internal_interface] = tunnel
    logging.info('Tunnel was added to the pool')
    return tunnel


def open_tunnels(context, hostnames, port, ssh_username):
    for hostname in hostnames:
        open_tunnel(context, hostname, port, ssh_username)


def get_bastion_host():
    """
    Return bastion host from env if exists
    """
    return os.environ.get('BASTION', 'lb.bastion.cloud.yandex.net')


def _ssh_options():
    options = ['-v', '-o', 'StrictHostKeyChecking=no']
    bastion = get_bastion_host()
    if bastion:
        return options + ['-J', bastion]
    return options


def ssh(
    host: str,
    command: List[str],
    user: str = 'root',
    stdout: int = subprocess.PIPE,
    stderr: int = subprocess.PIPE,
    options: List[str] = None,
    timeout: Union[int, None] = 300,
) -> (int, str, str):
    """
    Execute command on remote host
    Return Tuple of (return_code, stdout, stderr)
    """
    logging.debug(f'Executing command {command} on {host}')
    command = ' '.join(command)
    opts = _ssh_options()
    if options:
        opts += options
    args = ['ssh'] + opts + [f'{user}@{host}', command]
    ret = subprocess.run(args, stdout=stdout, stderr=stderr, timeout=timeout)
    return ret.returncode, ret.stdout, ret.stderr


def ssh_async(
    host: str,
    command: List[str],
    user: str = 'root',
    stdout: int = subprocess.PIPE,
    stderr: int = subprocess.PIPE,
    options: List[str] = None,
) -> (int, str, str):
    """
    Execute async command on remote host
    Returns Popen object
    """
    logging.debug(f'Executing command {command} on {host}')
    command = ' '.join(command)
    opts = _ssh_options()
    if options:
        opts += options
    args = ['ssh'] + opts + [f'{user}@{host}', command]
    started_process = subprocess.Popen(args, stdout=stdout, stderr=stderr)
    return started_process


def ssh_checked(
    host: str,
    command: List[str],
    user: str = 'root',
    message: str = None,
    options: List[str] = None,
    timeout: Union[int, None] = 300,
) -> (int, str, str):
    """
    Execute command on remote host and throw exception if fails
    Return stdout
    """
    message = message or f'Failed execute "{command}" on "{host}"'
    code, out, err = ssh(host, command, user, options=options, timeout=timeout)
    if code != 0:
        raise SSHError(f'{message}, stdout: {out}, stderr: {err}')
    return out


def _rsync_options():
    ssh = 'ssh -v -o StrictHostKeyChecking=no -l root'
    bastion = get_bastion_host()
    if bastion:
        ssh = f'{ssh} -A -J {bastion}'
    return ['--verbose', '--recursive', '--archive', '--compress', '-e', ssh]


def rsync(
    src: str,
    dst: str,
    user: str = 'root',
    additional_options: Sequence = (),
    chmod: Union[str, None] = None,
    owner: Union[str, None] = None,
    group: Union[str, None] = None,
    stdout: int = subprocess.PIPE,
    stderr: int = subprocess.PIPE,
    timeout: int = 120,
) -> (int, str, str):
    """
    Synchronize files using rsync
    """
    logging.info(f'rsyncing {src} to {dst}')
    cmd = ['rsync']
    if chmod:
        cmd.extend(['--chmod', chmod])
    if owner:
        cmd.append(f'--owner={owner}')
    if group:
        cmd.append(f'--group={group}')
    cmd.extend(_rsync_options())
    if len(additional_options):
        cmd.extend(additional_options)
    cmd.extend([src, dst])
    ret = subprocess.run(cmd, stdout=stdout, stderr=stderr, timeout=timeout)
    return ret.returncode, ret.stdout, ret.stderr


@retry(wait_fixed=5000, stop_max_attempt_number=4)
def rsync_checked(
    src: str,
    dst: str,
    user: str = 'root',
    additional_options: Sequence = (),
    chmod: Union[str, None] = None,
    owner: Union[str, None] = None,
    group: Union[str, None] = None,
    message: str = None,
    timeout: int = 120,
):
    message = message or f'Can\'t rsync from "{src}" to "{dst}"'
    code, out, err = rsync(src, dst, user, additional_options, chmod, owner, group, timeout=timeout)
    if code != 0:
        raise RsyncError(f'{message}, stdout: {out}, stderr: {err}')


def download(
    src: str, dst: str, stdout: int = subprocess.PIPE, stderr: int = subprocess.PIPE, timeout: int = 60
) -> (int, str, str):
    """
    Download files from remote host
    """
    logging.debug(f'downloading {src} to {dst}')
    cmd = ['rsync'] + _rsync_options() + [f'{src}', f'{dst}']
    ret = subprocess.run(cmd, stdout=stdout, stderr=stderr, timeout=timeout)
    return ret.returncode, ret.stdout, ret.stderr


def remote_file_write(
    data: Union[str, bytes],
    fqdn: str,
    path: str,
    mode: str = 'w',
    chmod: Union[str, None] = None,
    owner: Union[str, None] = None,
    group: Union[str, None] = None,
) -> None:
    """
    Write data to fqdn:path
    """
    with tempfile.NamedTemporaryFile(mode=mode) as temp_file:
        temp_file.write(data)
        temp_file.flush()
        return rsync(temp_file.name, f"{fqdn}:{path}", chmod=chmod, owner=owner, group=group)


def remote_yaml_write(data: dict, fqdn: str, path: str) -> None:
    """
    Write yaml config to fqdn:path
    """
    with tempfile.NamedTemporaryFile(mode='w') as temp_file:
        yaml.safe_dump(data, temp_file, default_flow_style=False)
        temp_file.flush()
        return rsync(temp_file.name, f'{fqdn}:{path}')


def get_inner_dict(parent_dict, path):
    elements = path.split('.')
    current_dict = parent_dict
    for element in elements:
        if element not in current_dict:
            current_dict[element] = {}
        current_dict = current_dict[element]
    return current_dict


class FakeContext:
    """
    Emulates behave context
    """

    def __init__(self, state, conf):
        self.state = state
        self.conf = conf
        self.folder = conf['dynamic']['folders'].get('test')
        self.cluster_type = 'hadoop'

    def __getattr__(self, attr):
        return self.__dict__[attr]

    def __setattr__(self, attr, value):
        self.__dict__[attr] = value
        return

    def __contains__(self, attr):
        return attr in self.__dict__


def combine_dict(dict1, dict2):
    """
    This (relatively) simple function performs
    deep-merge of 2 dicts returning new dict
    """

    def _update(orig, update):
        for key, value in update.items():
            if isinstance(value, collections.abc.Mapping):
                orig[key] = _update(orig.get(key, {}), value)
            else:
                orig[key] = update[key]
        return orig

    _result = {}
    _update(_result, dict1)
    _update(_result, dict2)
    return _result
