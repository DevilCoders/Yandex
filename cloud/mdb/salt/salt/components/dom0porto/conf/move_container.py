#!/usr/bin/env python
"""
Script for moving container between dom0 hosts
"""

import argparse
import json
import logging
import os
import re
import shlex
import subprocess
import sys
import time

import porto
import requests
import urllib3
from ConfigParser import ConfigParser
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

try:
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
except AttributeError:
    pass

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
LOG = logging.getLogger('move-container')

PORTO_CONN = porto.Connection()
PORTO_CONN.connect()

STATE_CACHE_DIR = '/var/cache/porto_agent_states'
TEMPLATE_PORTO_AGENT_MARK = STATE_CACHE_DIR + '/{fqdn}.{suff}'
DOM0_SALT_CMD = ('salt-call state.sls components.dom0porto.containers '
                 'pillar=\'{{"target-container": "{fqdn}"}}\' '
                 'concurrent=True --state-output=changes -l quiet')
CONT_SALT_CMD = (r"sed -i -e 's/.*yandex\.net$//g' -e '/^\s*$/d' /etc/hosts",
                 'salt-call saltutil.refresh_grains refresh_pillar=False',
                 'salt-call state.sls components.common.etc-hosts '
                 'concurrent=True --state-output=changes -l quiet',
                 'sudo -u selfdns /usr/bin/selfdns-client --debug --terminal '
                 '|| /bin/true')


def setup_session():
    """
    Create requests session with retries and connection pooling
    """
    session = requests.Session()
    retry = Retry(
        total=3,
        read=3,
        connect=3,
        backoff_factor=0.3,
        status_forcelist=(500, 502, 504),
    )
    adapter = HTTPAdapter(pool_connections=1, pool_maxsize=1, max_retries=retry)
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    return session


SESSION = setup_session()


def parse_args():
    """
    Function for parsing arguments
    """
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument('container', help='container to move')
    parser.add_argument('new_dom0', help='dom0-host where to move', nargs='?')
    parser.add_argument('--pre-stop-state', help='run salt state before container stop', default='')
    parser.add_argument('--force', action='store_true', help='skip container presence check')
    parser.add_argument('--progress', action='store_true', help='show rsync progress')
    parser.add_argument('--revert', action='store_true', help='revert transfer')

    args = parser.parse_args()

    return args


def get_config():
    """
    Simple fuction to read ~/.mdb_move_container.conf
    """
    config = ConfigParser()
    config.read(os.path.expanduser('~/.mdb_move_container.conf'))
    return config


def set_downtime(config, fqdn, duration=8 * 3600):
    """
    Set downtime via juggler-api
    """
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('juggler', 'token')),
        'Content-Type': 'application/json',
    }
    now = int(time.time())
    end = now + int(duration)
    data = json.dumps({
        'end_time': end,
        'start_time': now,
        'description': 'container move',
        'filters': [{
            'host': fqdn,
        }],
    })
    LOG.info('Setting downtime')
    resp = SESSION.post(
        config.get('juggler', 'url'),
        headers=headers,
        data=data,
    )
    if resp.status_code != 200:
        LOG.error('Could not set downtime (%s: %s)', resp.status_code, resp.text)


def remote_mkdir(dom0, path):
    """
    Create directory on remote host
    """
    cmd = [
        'ssh',
        'root@{dom0}'.format(dom0=dom0),
        'mkdir -p {path}'.format(path=path),
    ]
    LOG.info('Creating directory %s on %s', path, dom0)
    code = subprocess.Popen(cmd).wait()
    if code != 0:
        raise Exception('Could not create directory {dir} on host ' '{dom0}'.format(dir=path, dom0=dom0))


def run_local_state(container):
    """
    Run salt state locally
    """
    cmd = shlex.split(DOM0_SALT_CMD.format(fqdn=container))
    LOG.info('Running: %s', ' '.join(cmd))
    code = subprocess.Popen(cmd).wait()
    if code != 0:
        raise Exception('Could not run salt state locally')


def run_state_in_local_container(fqdn):
    """
    Run salt states inside porto container
    """
    # We do not need to update dns
    for cont_cmd in CONT_SALT_CMD[:-1]:
        cmd = ['portoctl', 'shell', fqdn, cont_cmd]
        LOG.info('Running: %s', ' '.join(cmd))
        code = subprocess.Popen(cmd).wait()
        if code != 0:
            raise Exception('Could not run salt state in {fqdn}'.format(fqdn=fqdn))


def run_pre_stop_state(fqdn, pre_stop_state):
    """
    Run pre stop salt state inside porto container
    """
    state_cmd = ('salt-call state.sls {state} concurrent=True '
                 '--state-output=changes -l quiet --retcode-passthrough').format(state=pre_stop_state)
    cmd = ['portoctl', 'shell', fqdn, state_cmd]
    LOG.info('Running: %s', ' '.join(cmd))
    code = subprocess.Popen(cmd).wait()
    if code != 0:
        raise Exception('Could not run pre stop salt state in {fqdn}'.format(fqdn=fqdn))


def run_state(dom0, container):
    """
    Run salt state on dom0 host
    """
    cmd = [
        'ssh',
        'root@{dom0}'.format(dom0=dom0),
        DOM0_SALT_CMD.format(fqdn=container),
    ]
    LOG.info('Running: %s', ' '.join(cmd))
    code = subprocess.Popen(cmd).wait()
    if code != 0:
        raise Exception('Could not run salt state on {dom0}'.format(dom0=dom0))


def run_state_in_container(dom0, fqdn):
    """
    Run salt states inside porto container
    """
    for cont_cmd in CONT_SALT_CMD:
        cmd = [
            'ssh',
            'root@{dom0}'.format(dom0=dom0),
            'portoctl shell {fqdn} {salt}'.format(fqdn=fqdn, salt=cont_cmd),
        ]
        LOG.info('Running: %s', ' '.join(cmd))
        code = subprocess.Popen(cmd).wait()
        if code != 0:
            raise Exception('Could not run salt state in {fqdn}'.format(fqdn=fqdn))


def rsync_dir(dom0, src_path, dst_path, bwlimit, show_progress):
    """
    Rsync local dir to remote host
    """
    cmd = [
        'rsync',
        '--bwlimit={bwlimit}'.format(bwlimit=bwlimit),
        '-aH',
        '--numeric-ids',
        '{path}/'.format(path=src_path),
        'root@{dom0}:{path}/'.format(dom0=dom0, path=dst_path),
    ]
    if show_progress:
        cmd.insert(2, '--progress')
    LOG.info('Running: %s', ' '.join(cmd))
    code = subprocess.Popen(cmd).wait()
    if code != 0:
        raise Exception('Could not rsync {dir} to host ' '{dom0}'.format(dir=src_path, dom0=dom0))


def get_transfer_data(config, transfer_id, allow_missing=False):
    """
    Get transfer info from dbm api
    """
    url = '{base}/api/v2/transfers/{transfer_id}'.format(base=config.get('dbm', 'url'), transfer_id=transfer_id)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('dbm', 'token')),
    }
    LOG.info('Getting transfer info for %s', transfer_id)
    res = SESSION.get(url, headers=headers, verify='/opt/yandex/allCAs.pem', timeout=5)
    if res.status_code == 404 and allow_missing:
        return {}
    if res.status_code >= 400:
        raise RuntimeError('DBM response {code}: {text}'.format(code=res.status_code, text=res.text))
    return res.json()


def get_transfer_data_by_container(config, fqdn):
    """
    Get transfer info from dbm api
    """
    url = '{base}/api/v2/transfers/?fqdn={fqdn}'.format(base=config.get('dbm', 'url'), fqdn=fqdn)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('dbm', 'token')),
    }
    LOG.info('Getting transfer info for %s', fqdn)
    res = SESSION.get(url, headers=headers, verify='/opt/yandex/allCAs.pem', timeout=5)
    if res.status_code >= 400:
        raise RuntimeError('DBM response {code}: {text}'.format(code=res.status_code, text=res.text))
    return res.json()


def get_volumes(config, fqdn):
    """
    Get container volumes from dbm api
    """
    url = '{base}/api/v2/volumes/{fqdn}'.format(base=config.get('dbm', 'url'), fqdn=fqdn)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('dbm', 'token')),
    }
    LOG.info('Getting volumes for %s', fqdn)
    res = SESSION.get(url, headers=headers, verify='/opt/yandex/allCAs.pem', timeout=5)
    if res.status_code >= 400:
        raise RuntimeError('DBM response {code}: {text}'.format(code=res.status_code, text=res.text))
    return res.json()


def prepare_transfer(config, fqdn, dom0):
    """
    Initiate transfer of container to dom0
    """
    url = '{base}/api/v2/containers/{fqdn}'.format(base=config.get('dbm', 'url'), fqdn=fqdn)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('dbm', 'token')),
        'Accept': 'application/json',
        'Content-Type': 'application/json',
    }
    data = json.dumps({'dom0': dom0})
    LOG.info('Preparing transfer for %s', fqdn)
    res = SESSION.post(url, headers=headers, data=data, verify='/opt/yandex/allCAs.pem', timeout=5)
    if res.status_code >= 400:
        raise RuntimeError('DBM response {code}: {text}'.format(code=res.status_code, text=res.text))
    return get_transfer_data(config, res.json()['transfer'])


def finish_transfer(config, transfer_id):
    """
    Finish transfer with dbm api
    """
    url = '{base}/api/v2/transfers/{transfer_id}/finish'.format(base=config.get('dbm', 'url'), transfer_id=transfer_id)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('dbm', 'token')),
    }
    LOG.info('Finishing transfer %s', transfer_id)
    res = SESSION.post(url, headers=headers, verify='/opt/yandex/allCAs.pem', timeout=5)
    if res.status_code >= 400:
        raise RuntimeError('DBM response {code}: {text}'.format(code=res.status_code, text=res.text))


def cancel_transfer(config, transfer_id):
    """
    Cancel transfer with dbm api
    """
    url = '{base}/api/v2/transfers/{transfer_id}/cancel'.format(base=config.get('dbm', 'url'), transfer_id=transfer_id)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=config.get('dbm', 'token')),
    }
    LOG.info('Cancelling transfer %s', transfer_id)
    res = SESSION.post(url, headers=headers, verify='/opt/yandex/allCAs.pem', timeout=5)
    if res.status_code >= 400:
        raise RuntimeError('DBM response {code}: {text}'.format(code=res.status_code, text=res.text))


def get_container(fqdn, ignore_presence=False):
    """
    Get container with local porto api
    """
    try:
        return PORTO_CONN.Find(fqdn)
    except porto.exceptions.ContainerDoesNotExist:
        if not ignore_presence:
            msg = 'Could not find container {fqdn}\n'
            msg += 'If you running the script after it was interrupted (due to reboot) use --force flag'
            raise Exception(msg.format(fqdn=fqdn))
    return None


def get_net_limit(container):
    """
    Get default net limit on container
    """
    res = {}
    for limit in container.GetProperty('net_limit').split(';'):
        label, value = limit.split(':')
        res[label] = int(value.strip()) / 1024

    return res.get('default')


def get_context(config, args):
    """
    Check if container exists and has transfer context.
    Initiate new transfer if there is no context saved.
    """
    fqdn = args.container
    data = get_transfer_data_by_container(config, fqdn)
    container = get_container(fqdn, ignore_presence=args.force)
    context_path = os.path.join(os.path.expanduser('~'), 'transfer-context-{fqdn}'.format(fqdn=fqdn))
    context = {}
    if os.path.exists(context_path):
        try:
            with open(context_path) as context_file:
                context = json.load(context_file)
        except Exception as exc:
            print('Unable to load transfer context: %s', repr(exc))
    if context:
        if ('transfer' not in context) and ('dom0' not in context):
            raise Exception('Malformed context: {context}'.format(context=context))
        if ('transfer' in context) and \
                (not get_transfer_data(config, context['transfer']['id'], allow_missing=True)) and \
                data:
            context['transfer'] = data
    else:
        if data:
            context['transfer'] = data
        elif args.revert:
            raise Exception('No active transfer. Unable to revert')
        else:
            context['transfer'] = prepare_transfer(config, args.container, args.new_dom0)
        context['path'] = context_path
        save_context(context)

    return container, context


def save_context(context):
    """
    Atomically save context
    """
    LOG.info('Saving context to %s', context['path'])
    tmp_path = context['path'] + '.tmp'
    with open(tmp_path, 'w') as out:
        json.dump(context, out)
    os.rename(tmp_path, context['path'])


def prepare_path_map(config, context):
    """
    Prepare local dom0 for transfering data
    """
    LOG.info('Preparing path map')
    src_volumes = get_volumes(config, context['transfer']['container'])
    dest_volumes = get_volumes(config, context['transfer']['placeholder'])
    path_map = {}
    for volume in src_volumes:
        path = volume['path']
        found = False
        for dest_volume in dest_volumes:
            if dest_volume['path'] == path:
                path_map[path] = {
                    'src_dom0_path':
                        volume['dom0_path'],
                    'dest_dom0_path':
                        dest_volume['dom0_path'].replace(context['transfer']['placeholder'],
                                                         context['transfer']['container']),
                }
                found = True
                break
        if not found:
            # Welcome to hell
            raise Exception('Unable to find {path} on destination {dest}'.format(
                path=path, dest=context['transfer']['placeholder']))
    # We need to move (or find already moved) data paths on source dom0
    for pair in path_map.values():
        new_path = '{path}.{ts}.bak'.format(path=pair['src_dom0_path'], ts=int(time.time()))
        if os.path.exists(pair['src_dom0_path']):
            os.rename(pair['src_dom0_path'], new_path)
        else:
            found = False
            for name in os.listdir(os.path.dirname(pair['src_dom0_path'])):
                if re.match('^{base}.[0-9]+.bak$'.format(base=os.path.basename(pair['src_dom0_path'])), name):
                    os.rename(os.path.join(os.path.dirname(pair['src_dom0_path']), name), new_path)
                    found = True
                    break
            if not found:
                raise Exception('Unable to find {path} locally'.format(path=pair['src_dom0_path']))
        pair['src_dom0_path'] = new_path
    return path_map


def revert_local_volumes(config, context):
    """
    Move local volumes back (if needed)
    """
    volumes = get_volumes(config, context['transfer']['container'])
    for volume in volumes:
        path = volume['dom0_path']
        if os.path.exists(path):
            continue
        for name in os.listdir(os.path.dirname(path)):
            if re.match('^{base}.[0-9]+.bak$'.format(base=os.path.basename(path)), name):
                src_path = os.path.join(os.path.dirname(path), name)
                LOG.info('Moving %s back to %s', src_path, path)
                os.rename(src_path, path)
                break


def drop_dest_volumes(config, context):
    """
    Drop volumes on destination dom0
    """
    volumes = get_volumes(config, context['transfer']['placeholder'])
    for volume in volumes:
        cmd = [
            'ssh',
            'root@{dom0}'.format(dom0=context['transfer']['dest_dom0']),
            'rm -rf {path}'.format(path=volume['dom0_path']),
        ]
        LOG.info('Dropping directory %s on %s', volume['dom0_path'], context['transfer']['dest_dom0'])
        code = subprocess.Popen(cmd).wait()
        if code != 0:
            raise Exception('Could not drop directory {dir} on host {dom0}'.format(
                dir=volume['dom0_path'], dom0=context['transfer']['dest_dom0']))


def ensure_porto_agent_mark(container, transfer_info):
    """
    Set flag for container to skip start on dom0 reboot
    """
    mark_path = TEMPLATE_PORTO_AGENT_MARK.format(fqdn=container, suff="extra")
    if os.path.exists(mark_path):
        return
    extra = {'operation': 'move', 'dest_dom0': transfer_info.get('dest_dom0', 'unknown')}
    with open(mark_path, 'w') as out:
        json.dump(extra, out)


def safe_unlink(path):
    """
    Unlink path only if it exists
    """
    if os.path.exists(path):
        os.unlink(path)


def transfer(config, container, context, args):
    """
    Perform transfer
    """
    fqdn = args.container
    if 'transfer' in context:
        data = get_transfer_data(config, context['transfer']['id'], allow_missing=True)
        if data:
            bwlimit = 102400 # 100 MB/s
            set_downtime(config, fqdn)
            if os.path.isdir(STATE_CACHE_DIR):
                ensure_porto_agent_mark(fqdn, context['transfer'])
            if container:
                if args.pre_stop_state and container.GetProperty('state') == 'running':
                    run_pre_stop_state(fqdn, args.pre_stop_state)
                bwlimit = get_net_limit(container) or bwlimit
                container.Stop()
            path_map = prepare_path_map(config, context)
            for pair in path_map.values():
                if not pair['dest_dom0_path'].startswith('/disks'):
                    remote_mkdir(context['transfer']['dest_dom0'], os.path.dirname(pair['dest_dom0_path']))
                rsync_dir(context['transfer']['dest_dom0'], pair['src_dom0_path'], pair['dest_dom0_path'],
                          bwlimit, args.progress)
            finish_transfer(config, context['transfer']['id'])
        context['dom0'] = context['transfer']['dest_dom0'][:]
        context.pop('transfer')
        save_context(context)

    run_state(context['dom0'], fqdn)
    run_state_in_container(context['dom0'], fqdn)
    LOG.info('Sleeping for 5 minutes before container removal')
    time.sleep(300)
    if container:
        container.Destroy()
    safe_unlink(TEMPLATE_PORTO_AGENT_MARK.format(fqdn=fqdn, suff="json"))
    safe_unlink(TEMPLATE_PORTO_AGENT_MARK.format(fqdn=fqdn, suff="extra"))
    os.unlink(context['path'])


def revert(config, container, context, args):
    """
    Revert transfer
    """
    if 'transfer' not in context:
        raise Exception('No active transfer. Unable to revert')
    fqdn = args.container
    revert_local_volumes(config, context)
    drop_dest_volumes(config, context)
    safe_unlink(TEMPLATE_PORTO_AGENT_MARK.format(fqdn=fqdn, suff="extra"))
    cancel_transfer(config, context['transfer']['id'])
    os.unlink(context['path'])
    run_local_state(fqdn)
    run_state_in_local_container(fqdn)


def _main():
    if not os.environ.get('SSH_AUTH_SOCK'):
        print('No ssh agent. Exiting')
        sys.exit(1)
    args = parse_args()
    config = get_config()
    container, context = get_context(config, args)
    if args.revert:
        return revert(config, container, context, args)
    return transfer(config, container, context, args)


if __name__ == '__main__':
    _main()
