"""
DBaaS Instance Billing reporter
"""

import calendar
import json
import os
import time
import uuid
from collections import namedtuple
from logging import Formatter, getLogger
from logging.handlers import RotatingFileHandler

import requests

METADATA_PATH = 'http://169.254.169.254/computeMetadata/v1/instance/attributes/billing-disabled'
STATE_PATH = '/tmp/.billing.state'
MONRUN_CACHE_PATH = '/var/cache/monrun'

_initialized = False

BILLING_LOGGER = getLogger(__name__ + '.billing')
SERVICE_LOGGER = getLogger(__name__ + '.service')
BILLING_STATE_TTL = 300

REQUIRED_PARAMS = [
    'cluster_id',
    'folder_id',
    'cloud_id',
    'resource_preset_id',
    'platform_id',
    'cores',
    'core_fraction',
    'memory',
    'io_cores_limit',
    'disk_type_id',
    'disk_size',
    'roles',
    'compute_instance_id',
]

MonrunCheck = namedtuple('MonrunCheck', ['name', 'alive', 'body'])


def is_disabled_by_metadata():
    """
    Check if we have metadata flag to not bill current instance
    """
    try:
        res = requests.get(METADATA_PATH, headers={'Metadata-Flavor': 'Google'}, timeout=1)
        res.raise_for_status()
        if res.text == 'true':
            return True
    except Exception:
        pass
    return False


def get_monrun_checks(checks):
    """
    Get monrun checks for this host based on runlist
    """
    for check, timeout in checks.items():
        suffix = '_{check}'.format(check=check)
        check_body = None
        for name in os.listdir(MONRUN_CACHE_PATH):
            if name.endswith(suffix):
                path = os.path.join(MONRUN_CACHE_PATH, name)
                mtime = os.stat(path).st_mtime
                if time.time() - mtime < timeout:
                    with open(path) as check_file:
                        check_body = check_file.read()
                        break

        if not check_body:
            yield MonrunCheck(name=check, alive=False, body='no fresh check found')
        else:
            yield MonrunCheck(name=check, alive=check_body[0] == '0', body=check_body)


class BillingFormatter(Formatter):
    """
    Billing record formatter
    """

    def format(self, record):
        """
        Format record to billing-specific format
        """
        fields = list(vars(record))
        data = {name: getattr(record, name, None) for name in fields}

        usage = {
            'type': 'delta',
            'quantity': data['ts'] - data['last_ts'],
            'unit': 'seconds',
            'start': data['last_ts'],
            'finish': data['ts'],
        }

        tags = dict(
            resource_preset_id=data['params']['resource_preset_id'],
            platform_id=data['params']['platform_id'],
            cores=data['params']['cores'],
            core_fraction=data['params']['core_fraction'],
            memory=data['params']['memory'],
            software_accelerated_network_cores=data['params']['io_cores_limit'],
            cluster_id=data['params']['cluster_id'],
            cluster_type=data['params']['cluster_type'],
            disk_type_id=data['params']['disk_type_id'],
            disk_size=data['params']['disk_size'],
            public_ip=data['params']['assign_public_ip'],
            roles=data['params']['roles'],
            compute_instance_id=data['params']['compute_instance_id'],
            online=1,
            on_dedicated_host=data['params']['on_dedicated_host'],
        )
        edition = data['params'].get('edition')
        if edition:
            tags['edition'] = edition

        ret = dict(
            schema='mdb.db.generic.v1',
            source_wt=data['ts'],
            source_id=data['params']['fqdn'],
            resource_id=data['params']['fqdn'],
            folder_id=data['params']['folder_id'],
            cloud_id=data['params']['cloud_id'],
            usage=usage,
            tags=tags,
            id=str(uuid.uuid4()),
        )
        return json.dumps(ret)


def init_logger(log_file, service_log_file, rotate_size, backup_count):
    """
    Init billing logger
    """
    global _initialized  # pylint: disable=global-statement

    if _initialized:
        return  # pragma: nocover

    handler = RotatingFileHandler(log_file, maxBytes=rotate_size, backupCount=backup_count)
    formatter = BillingFormatter()
    handler.setFormatter(formatter)
    BILLING_LOGGER.addHandler(handler)

    service_handler = RotatingFileHandler(service_log_file, maxBytes=rotate_size, backupCount=backup_count)

    service_handler.setFormatter(Formatter(fmt='%(asctime)s [%(levelname)s]: %(message)s'))
    SERVICE_LOGGER.addHandler(service_handler)
    _initialized = True


class BillingState(object):
    """
    Billing state.
    """

    def __init__(self, last_ts, alive):
        self.last_ts = last_ts
        self.alive = alive

    def as_dict(self):
        return {'last_ts': self.last_ts, 'alive': self.alive}

    def __repr__(self):
        return 'BillingState(last_ts=%r, alive=%r)' % (self.last_ts, self.alive)


def get_state():
    """
    Get last saved state for delta report
    """
    try:
        with open(STATE_PATH) as fobj:
            return BillingState(**json.load(fobj))
    except Exception as exc:  # pylint: disable=broad-except
        SERVICE_LOGGER.warning('Unable to get_state, cause: %s', exc)
        return None


def save_state(state):
    """
    Atomically save state in file
    """
    tmp_path = '{base}.tmp'.format(base=STATE_PATH)
    with open(tmp_path, 'w') as fobj:
        fobj.write(json.dumps(state.as_dict()))
    os.rename(tmp_path, STATE_PATH)


def write_billing_log(last_ts, params, current_ts):
    """
    Write billing log.

    Split onto 2 messages if hour changed within billing interval
    """
    last_tuple = time.gmtime(last_ts)
    current_tuple = time.gmtime(current_ts)
    if last_tuple.tm_hour != current_tuple.tm_hour:
        split_ts = calendar.timegm(
            (current_tuple.tm_year, current_tuple.tm_mon, current_tuple.tm_mday, current_tuple.tm_hour, 0, 0,
             current_tuple.tm_wday, current_tuple.tm_yday, current_tuple.tm_isdst))
        BILLING_LOGGER.info('', extra=dict(params=params, last_ts=last_ts, ts=split_ts))
        last_ts = split_ts
    BILLING_LOGGER.info('', extra=dict(params=params, last_ts=last_ts, ts=current_ts))


NONE_STATE = BillingState(last_ts=0, alive=False)


def get_previous_state(current_ts):
    """
    Get previous state.

    Verify that it not outdated
    """
    state = get_state()

    if state is None:
        SERVICE_LOGGER.debug('No previous state')
        return NONE_STATE

    if state.last_ts + BILLING_STATE_TTL < current_ts:
        SERVICE_LOGGER.warning('Billing state outdated: %r, current_ts: %r', state, current_ts)
        return NONE_STATE

    return state


def compute_current_state(current_ts, checks):
    """
    Compute current state from checks.
    """
    current_state = BillingState(last_ts=current_ts, alive=True)
    for monrun_check in get_monrun_checks(checks):
        if not monrun_check.alive:
            current_state.alive = False
            SERVICE_LOGGER.warning('%s check failed: %s', monrun_check.name, monrun_check.body)
    return current_state


def billing(  # pylint: disable=too-many-arguments
        log_file, service_log_file, rotate_size, params, checks, backup_count=1):
    """
    Run billing check (use this as dbaas-cron target function)
    """
    for key, val in params.items():
        if key in REQUIRED_PARAMS and val is None:
            SERVICE_LOGGER.warning('Malformed params, %r is None', key)
            return

    if not _initialized:
        init_logger(
            log_file=log_file, service_log_file=service_log_file, rotate_size=rotate_size, backup_count=backup_count)

    current_ts = int(time.time())

    previous_state = get_previous_state(current_ts)
    current_state = compute_current_state(current_ts, checks)

    if current_state.alive and previous_state.alive:
        if is_disabled_by_metadata():
            SERVICE_LOGGER.info('Not billed cause: disabled by metadata')
        else:
            write_billing_log(previous_state.last_ts, params, current_ts)
    else:
        SERVICE_LOGGER.info('Not billed cause: currently alive: %r, previously alive: %r', current_state.alive,
                            previous_state.alive)

    save_state(current_state)
