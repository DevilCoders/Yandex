# coding: utf-8
"""
Lockbox module for salt

## Requirements:

1. Minion configuration with Lockbox payload endpoint:


    $ cat /etc/salt/minion.d/lockbox.conf
    lockbox:
        address: https://payload.lockbox.api.cloud-preprod.yandex.net/lockbox

2. VM with IPV4 metadata enabled on it.
3. Service account accessible via metadata with lockbox.payloadViewer permission to the secrets.


    $ yc lockbox secret add-access-binding $SECRET_ID --role 'lockbox.payloadViewer' --service-account-id $SA_ID


"""


import json
import logging
import sys


try:
    from salt.exceptions import CommandExecutionError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

    class CommandExecutionError(RuntimeError):
        pass


def __virtual__():
    if sys.version_info[0] == 2:
        return (
            False,
            'The lockbox execution module cannot be loaded: it doesn\'t support Python2',
        )
    return True


log = logging.getLogger(__name__)

# For linters, salt will populate it in runtime
__opts__ = {}
__context__ = {}
__salt__ = {}


def _get_lockbox_opts():
    opts = __opts__.get('lockbox')
    if not opts or 'address' not in opts:
        raise CommandExecutionError("Lockbox module not configured. 'lockbox:address' is unset")
    return opts


def _parse_payload_response(js_resp):
    """
    Convert payload response to the dict
    """
    from base64 import decodebytes

    ret = {}
    for entry in js_resp['entries']:
        if 'text_value' in entry:
            value = entry['text_value']
        else:
            # binary values are base64 encoded
            value = decodebytes(entry['binary_value'].encode('ascii')).decode('ascii')
        ret[entry['key']] = value

    return ret


def get(secret_id):
    """
    Get secret payload from Lockbox.
    Expect that:
    - Lockbox address is config.
    - VM has a service account that is accessible via metadata.

    CLI Example::

        salt-call lockbox.get secret_id
    """
    lockbox_opts = _get_lockbox_opts()

    token = __salt__['compute_metadata.iam_token']()
    ret = __salt__['cmd.run_all'](
        [
            'grpcurl',
            '-rpc-header',
            'Authorization: Bearer ' + token,
            '-d',
            json.dumps({'secret_id': secret_id}),
            lockbox_opts['address'],
            'yandex.cloud.priv.lockbox.v1.PayloadService/Get',
        ]
    )

    if ret['retcode'] != 0:
        raise CommandExecutionError('get secret payload: {}'.format(ret['stderr']))

    js_resp = json.loads(ret['stdout'])
    log.debug("got '%s' secret it's version is '%s'", secret_id, js_resp['version_id'])

    return _parse_payload_response(js_resp)
