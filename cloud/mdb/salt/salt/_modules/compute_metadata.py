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

import time
import logging

try:
    from salt.exceptions import CommandExecutionError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

    class CommandExecutionError(RuntimeError):
        pass


try:
    import requests

    HAS_REQUESTS = True
except ImportError:
    HAS_REQUESTS = False

log = logging.getLogger(__name__)

# For linters, salt will populate it in runtime
__opts__ = {}
__context__ = {}
__salt__ = {}


def __virtual__():
    if not HAS_REQUESTS:
        return (
            False,
            'The compute_metadata execution module cannot be loaded: the request python library is not available.',
        )
    return True


_SESSION_KEY = 'compute_metadata.session'


def _get_session(retry_total=5):
    if _SESSION_KEY not in __context__:
        session = requests.Session()
        try:
            from urllib3.util.retry import Retry
            from requests.adapters import HTTPAdapter

            retry_strategy = Retry(
                total=retry_total,
                status_forcelist=[429, 500, 502, 503, 504],
            )
            adapter = HTTPAdapter(max_retries=retry_strategy)
            session.mount('https://', adapter)
            session.mount('http://', adapter)
        except ImportError as exc:
            log.warning('No retries for request %s. Continue without them', exc)
        __context__[_SESSION_KEY] = session
    return __context__[_SESSION_KEY]


def _get_iam_token_from_metadata():
    """
    Get IAM token from metadata
    """
    try:
        resp = _get_session().get(
            'http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token',
            timeout=5,
            headers={'Metadata-Flavor': 'Google'},
        )
    except Exception as exc:
        raise CommandExecutionError('failed to get IAM token from metadata: {}'.format(exc))
    if resp.status_code != 200:
        if resp.status_code == 404:
            raise CommandExecutionError(
                "metadata returns 404 for service account token, probably that VM doesn't have SA?"
            )
        raise CommandExecutionError('metadata request {} finished unsuccessfully: {}'.format(resp, resp.text))
    resp_js = resp.json()
    return resp_js['access_token'], resp_js['expires_in']


_TOKEN_EXP_GAP = 60
_TOKEN_KEY = 'compute_metadata.token'
_TOKEN_EXP_AT_KEY = 'compute_metadata.token_expires_at'


def iam_token():
    """
    Get IAM token
    """
    ctx = __context__
    if _TOKEN_EXP_AT_KEY in ctx and ctx[_TOKEN_EXP_AT_KEY] < (time.time() + _TOKEN_EXP_GAP):
        del ctx[_TOKEN_KEY]
        del ctx[_TOKEN_EXP_AT_KEY]
    if _TOKEN_KEY not in ctx:
        token, expires_in = _get_iam_token_from_metadata()
        ctx[_TOKEN_KEY] = token
        ctx[_TOKEN_EXP_AT_KEY] = time.time() + expires_in
    return ctx[_TOKEN_KEY]


def attribute(attr):
    """
    Get metadata attribute
    """
    try:
        resp = _get_session().get(
            'http://169.254.169.254/computeMetadata/v1/instance/attributes/' + attr,
            timeout=5,
            headers={'Metadata-Flavor': 'Google'},
        )
    except Exception as exc:
        raise CommandExecutionError('failed to get {} from metadata: {}'.format(attr, exc))
    if resp.status_code != 200:
        if resp.status_code == 404:
            log.info('There are not %s attribute in metadata', attr)
            return None
        raise CommandExecutionError('metadata {} request {} finished unsuccessfully: {}'.format(attr, resp, resp.text))
    return resp.text
