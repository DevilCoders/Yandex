# -*- coding: utf-8 -*-
"""
DBaaS module for salt
"""

from __future__ import absolute_import, print_function, unicode_literals

import datetime
import logging
import os
import re
import subprocess
import json

__salt__ = {}


LOG = logging.getLogger(__name__)
_default = object()

_PROD_ENVS = {
    'compute-prod',  # compute-prod
    'prod',  # porto-prod
}


def __virtual__():
    """
    We always return True here (we are always available)
    """
    return True


def pillar(key, default=_default):
    """
    Like __salt__['pillar.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised regardless of 'pillar_raise_on_missing' option.
    """
    value = __salt__['pillar.get'](key, default=default)
    if value is _default:
        raise KeyError('Pillar key not found: {0}'.format(key))

    return value


def grains(key, default=_default, **kwargs):
    """
    Like __salt__['grains.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised.
    """
    value = __salt__['grains.get'](key, default=default, **kwargs)
    if value is _default:
        raise KeyError('Grains key not found: {0}'.format(key))

    return value


def feature_flag(flag_name):
    """
    Check if feature flag is enabled
    """
    LOG.info('Checking if %s is enabled', flag_name)
    flags = __salt__['pillar.get']('feature_flags', {})
    LOG.debug('Enabled flags: %s', ','.join(sorted(flags)))
    ret = flag_name in flags
    LOG.debug('%s is %s', flag_name, 'enabled' if ret else 'disabled')
    return ret


def password2regex(password):
    """
    Build stupid regexp to cut password from logs
    """
    prefixlen = min(7, len(password) / 3)
    prefix = re.escape(password[:prefixlen])
    taillen = len(password) - prefixlen
    return '%s.{%d}' % (prefix, taillen)


def ping_minion():
    """
    Cross-platform function to ping salt-minion.
    """
    now = datetime.datetime.utcnow().strftime('%m/%d/%y %H:%M:%S')
    if os.name == 'nt':
        path = r'c:\salt\var\run\mdb-salt-ping-ts'
    else:
        path = r'/tmp/mdb-salt-ping-ts'
    with open(path, 'w') as fh:
        fh.write(now)
    return True


def installation():
    """
    Returns an installation name based on pillar data
    """
    return pillar('data:dbaas:vtype', 'porto')


def is_aws():
    """
    Returns true if installation() is `aws`
    """
    return installation() == 'aws'


def is_compute():
    """
    Returns true if installation() is `compute`
    """
    return installation() == 'compute'


def is_porto():
    """
    Returns true if installation() is `porto`
    """
    return installation() == 'porto'


def is_prod():
    """
    Returns true if environment is production
    """
    return pillar('yandex:environment', None) in _PROD_ENVS


def is_testing():
    """
    Returns true if environment is testing
    """
    return not is_prod()


def is_dataplane():
    """
    Return true if called from dataplane
    """
    return pillar('data:dbaas:cluster_id', None) is not None


def to_domain(domain):
    """
    Return local hostname suffixed with 'domain'
    """
    localpart, _, domain_orig = grains('id').partition('.')
    return '{local}.{domain}'.format(local=localpart, domain=domain)


def is_public_ca():
    """
    Return whether we don't need specify ca bundle.
    TODO: make special pillar key or something else
    """
    return is_aws()


def managed_hostname():
    """
    Return managed hostname
    """
    zone = pillar('data:dns:managed_zone', default='db.yandex.net')
    return to_domain(zone)


def data_mount_point():
    """
    Return data mount point
    """
    if 'components.zk' in pillar('data:runlist', []):
        directory = 'zookeeper'
    else:
        directory = pillar('data:dbaas:cluster_type').split('_')[0]
    return r'/var/lib/' + directory


def _extract_data_disk_from_lsblk_out(lsblk_out):
    disks_devices = [dev for dev in json.loads(lsblk_out)['blockdevices'] if dev['type'] == 'disk']
    if len(disks_devices) != 2:
        raise RuntimeError('Expect only two disk but got %d: %r', len(disks_devices), disks_devices)
    for dev in disks_devices:
        dev_name = dev['name']
        if 'children' in dev:
            if [part for part in dev['children'] if part['mountpoint'] == '/']:
                # root device, skip it
                continue
            # The device has partitions not mounted to the root that means it's a data device.
            if len(dev['children']) != 1:
                raise RuntimeError(
                    'Expect one partition on data device {data_dev_name}. But got: {partitions}'.format(
                        data_dev_name=dev['name'],
                        partitions=dev['children'],
                    )
                )
            partition = dev['children'][0]
            return {
                'device': {
                    'name': dev_name,
                    'size': dev['size'],
                },
                'partition': {
                    'name': partition['name'],
                    'size': partition['size'],
                    'exists': True,
                },
            }
        # Treat that device as a data disk:
        # - We create VMs with two disk
        # - If that disk doesn't have partitions - it's mean that its data disk
        if pillar('data:encryption:enabled', False):
            # If encryption is enabled, partition is decrypted disk.
            partition = '/dev/mapper/' + dev_name.split('/')[-1]
        else:
            partition = dev_name + 'p1' if dev_name[-1].isdigit() else '1'

        return {
            'device': {
                'name': dev_name,
                'size': dev['size'],
            },
            'partition': {
                'name': partition,
                'size': '0',
                'exists': False,
            },
        }


def data_disk():
    """
    Return data disk info for VM with network disk.

    Notes:
        - it is useless in porto.
        - doesn't support Windows
        - probably doesn't work correctly with local SSD
    """
    if installation() == 'porto' or os.name == 'nt':
        return {}

    lsblk_out = subprocess.check_output(['lsblk', '--paths', '--json', '--output=NAME,TYPE,MOUNTPOINT,SIZE'])
    return _extract_data_disk_from_lsblk_out(lsblk_out)


def common_component():
    """
    Return common component based on OS and installation
    """
    if os.name == 'nt':
        return 'common-windows'
    if is_aws():
        return 'datacloud.common'
    return 'common'


def fqdn():
    """
    Returns host fqdn
    """
    return pillar('data:dbaas:fqdn', grains('fqdn', 'unknown'))


def dc():
    """
    Returns host dc
    """
    return grains('ya:short_dc', fqdn()[:3])


def errata_warn_threshold():
    """
    Returns errata warn threshold
    """
    if is_dataplane():
        return 14

    env_delay = 0
    my_dc = dc()
    dc_to_delay = {'myt': 0, 'iva': 1, 'sas': 2, 'vla': 3}
    one_env_delay = max(dc_to_delay.values())
    dc_delay = dc_to_delay.get(my_dc, one_env_delay)
    # 4 days for env, 1 day for dc
    # porto-test > compute-preprod > porto-prod > compute-prod
    if is_compute():
        env_delay += one_env_delay + 1
    if is_prod():
        env_delay += (one_env_delay + 1) * 2

    return dc_delay + env_delay


def is_byoa():
    """
    Return true if BYOA enabled for that cluster
    """
    return pillar('data:byoa', None) is not None


def aws_apt_s3_role_arn():
    self_account_id = pillar('data:byoa:account_id')
    apt_s3_account_id = pillar('data:aws:apt_s3_account_id')
    self_role_arn = pillar('data:aws:role:arn')
    return self_role_arn.replace(self_account_id, apt_s3_account_id)
