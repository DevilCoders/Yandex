#!/usr/bin/python2
"""
Salt returner for mdb deploy
"""

import base64
import json
import logging
import os
import random
import socket
import time
import uuid

import requests
import yaml


if os.name == 'nt':
    CFG_ROOT = r'C:\salt\conf'
    CA_PATH = r'C:\ProgramData\Yandex\allCAs.pem'
    OVERRIDE_CA_PATH = r'C:\ProgramData\Yandex\overrideCAs.pem'
    STORE_PATH = r'C:\salt\var\run\mdb-salt-returner'
    JOB_RESULT_BLACKLIST_PATH = r'C:\salt\conf\job_result_blacklist.yaml'
else:
    CFG_ROOT = '/etc/yandex/mdb-deploy'
    CA_PATH = '/opt/yandex/allCAs.pem'
    OVERRIDE_CA_PATH = '/opt/yandex/overrideCAs.pem'
    STORE_PATH = '/var/lib/yandex/mdb-salt-returner'
    JOB_RESULT_BLACKLIST_PATH = '/etc/yandex/mdb-deploy/job_result_blacklist.yaml'


DEPLOY_VERSION_PATH = os.path.join(CFG_ROOT, 'deploy_version')
MDB_DEPLOY_API_HOST_PATH = os.path.join(CFG_ROOT, 'mdb_deploy_api_host')
SEND_SLEEPS = (
    1,
    2,
    4,
    8,
    16,
)
STORED_SEND_MIN_INTERVAL = 60
STORED_SEND_MAX_INTERVAL = 3600

LOG = logging.getLogger(__name__)


class DeployError(Exception):
    """
    Base deploy error
    """


class DeployConfigurationError(DeployError):
    """
    Configuration error
    """


class MalformedReturn(DeployError):
    """
    Unexpected return
    """


def _get_deploy_version():
    try:
        with open(DEPLOY_VERSION_PATH) as version:
            return int(version.read())
    except BaseException as ex:
        LOG.debug('Exception while reading deploy version: %s', repr(ex))
        return 1


def _get_deploy_api_host():
    try:
        with open(MDB_DEPLOY_API_HOST_PATH, 'r') as urlfile:
            host = urlfile.readline().strip()
            if not host:
                raise DeployConfigurationError(
                    'empty mdb deploy api url at {path}'.format(path=MDB_DEPLOY_API_HOST_PATH)
                )
            return host
    except IOError as exc:
        raise DeployConfigurationError('Unable to read deploy api file: {error}'.format(error=repr(exc)))


def _get_minion_id():
    try:
        return __grains__['id']  # noqa
    except KeyError as exc:
        raise DeployConfigurationError('Unable to get minion_id: {error}'.format(error=repr(exc)))


def _load_job_result_blacklist():
    try:
        with open(JOB_RESULT_BLACKLIST_PATH) as blacklist_file:
            return yaml.safe_load(blacklist_file)
    except BaseException as exc:
        LOG.debug('Failed to load job result blacklist: %s', repr(exc))


def _make_jitter(seconds=3):
    return float(random.randint(0, seconds * 100)) / 100.0


def is_return_blacklisted(ret):
    """
    Check if result is blacklisted.
    Code copies the one from https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/deploy/api/internal/core/service.go
    """
    func = ret['fun']
    args = ret.get('fun_args', None)
    blacklist = _load_job_result_blacklist()
    if not blacklist:
        return False

    for bjr in blacklist['blacklist']:
        bjr_func = bjr.get('func', None)
        bjr_args = bjr.get('args', None)

        if match_blacklist(func, args, bjr_func, bjr_args):
            return True

    return False


def match_blacklist(func, args, bjr_func, bjr_args):
    if bjr_func and func != bjr_func:
        return False

    if not bjr_args:
        return True

    if not args:
        return False

    for arg in args:
        if arg not in bjr_args:
            return False

    return True


def send_return(ret, minion_id=None):
    """
    Send return with retries
    """
    try:
        jid = ret['jid']
    except (KeyError, TypeError) as exc:
        raise MalformedReturn("No jid in return: {error}".format(error=repr(exc)))

    try:
        if minion_id is None:
            minion_id = _get_minion_id()
        host = _get_deploy_api_host()
    except DeployConfigurationError as exc:
        LOG.error("Unable to define deploy configuration: %s. Give up.", repr(exc))
        return False

    if is_return_blacklisted(ret):
        LOG.debug("Return is blacklisted. Not returning.")
        return True

    data = json.dumps({'result': base64.b64encode(json.dumps(ret).encode()).decode()})
    url = 'https://{host}/v1/minions/{minion_id}/jobs/{jid}/results'.format(host=host, minion_id=minion_id, jid=jid)

    try_num_with_sleep = iter(enumerate(SEND_SLEEPS))

    while True:
        rid = str(uuid.uuid4())
        headers = {
            'Content-Type': 'application/json',
            'X-Request-Id': rid,
        }
        LOG.debug('Sending job %s result to %s (request id is %s)', jid, url, rid)
        try:
            if os.path.exists(OVERRIDE_CA_PATH):
                ca_path = OVERRIDE_CA_PATH
            else:
                ca_path = CA_PATH
            res = requests.post(url, headers=headers, data=data, verify=ca_path, timeout=5)
            if res.status_code == 200:
                LOG.debug('Successfully sent job %s result', jid)
                return True

            LOG.error(
                'Failed to send job %s result, request id %s, status %r, response %s',
                jid,
                rid,
                res.status_code,
                res.text,
            )
        except (IOError, socket.error, socket.timeout) as exc:
            LOG.warning('Got communication error, while sending job %s: %s', jid, exc)

        try:
            try_no, sleep_seconds = next(try_num_with_sleep)
        except StopIteration:
            LOG.error('We fail and no more reties left')
            break

        sleep_seconds += _make_jitter()
        LOG.info('We fail at %d try. Sleep %d seconds', try_no, sleep_seconds)
        time.sleep(sleep_seconds)

    return False


def _store_return(ret):
    if not os.path.exists(STORE_PATH):
        os.makedirs(STORE_PATH)

    name = os.path.join(STORE_PATH, '{minion_id}__{jid}.json'.format(jid=ret['jid'], minion_id=_get_minion_id()))
    tmp_name = name + '.tmp'

    with open(tmp_name, 'w') as data:
        json.dump(ret, data)

    # Atomic 'write'
    os.rename(tmp_name, name)


def returner(ret):
    """
    Return job result to mdb deploy service
    See https://docs.saltstack.com/en/2019.2/ref/returners/ for interface specification
    """
    version = _get_deploy_version()
    if version == 3:
        try:
            _get_deploy_api_host()
        except DeployConfigurationError as exc:
            LOG.debug('Version is 3, but %s. Ignore return', exc)
            return
    elif version != 2:
        LOG.debug('Version is not 2: %r', version)
        return

    try:
        if send_return(ret):
            return
    except BaseException:
        LOG.exception("Got unexpected exception on return send. Store return to file.")

    _store_return(ret)


def send_stored_returns():
    """
    Get stored returns and retry send
    """
    if not os.path.exists(STORE_PATH):
        return

    now = time.time()

    for name in os.listdir(STORE_PATH):
        if not name.endswith('.json') or '__' not in name:
            LOG.debug('Skipping %s due to unexpected file name', name)
            continue
        minion_id, rest = name.split('__')
        jid = rest.split('.')[0]
        path = os.path.join(STORE_PATH, name)
        stat = os.stat(path)
        if now - stat.st_ctime < STORED_SEND_MIN_INTERVAL:
            continue
        if now - stat.st_mtime < min(stat.st_mtime - stat.st_ctime, STORED_SEND_MAX_INTERVAL):
            continue

        try:
            with open(path) as inp:
                ret = json.load(inp)
        except BaseException as exc:
            LOG.error('Unable to parse stored return: %s', repr(exc))
            os.unlink(path)
            continue

        try:
            if send_return(ret, minion_id=minion_id):
                os.unlink(path)
                continue
        except BaseException:
            LOG.exception('Got unexpected exception on %s job return send. Updating stored file mtime.', jid)

        os.utime(path, None)


if __name__ == '__main__':
    send_stored_returns()
