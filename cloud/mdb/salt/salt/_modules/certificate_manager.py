import json
import logging

try:
    from salt.exceptions import CommandExecutionError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

    class CommandExecutionError(RuntimeError):
        pass


__salt__ = {}
__opts__ = {}

log = logging.getLogger(__name__)


def _get_opts():
    opts = __opts__.get('certificate-manager')
    if not opts or 'address' not in opts:
        raise CommandExecutionError("certificate_manager module not configured. 'certificate-manager:address' is unset")
    return opts


def _parse_response(resp_text):
    resp = json.loads(resp_text)
    log.debug("got '%s' certificate it's version is '%s'", resp['certificate_id'], resp['version_id'])
    chain = resp['certificate_chain']
    return {'cert.crt': chain, 'cert.key': resp['private_key'], 'cert.ca': list(chain[1:])}


def get():
    """
    Get certificate
    Expect that:
    1. Instance has metadata
    2. Instance has service account
    3. In metadata there is `certificate-id` attribute
    """
    opts = _get_opts()
    certificate_id = __salt__['compute_metadata.attribute']('certificate-id')
    if not certificate_id:
        return CommandExecutionError('there are not certificate-id in metadata')
    token = __salt__['compute_metadata.iam_token']()
    ret = __salt__['cmd.run_all'](
        [
            'grpcurl',
            '-rpc-header',
            'Authorization: Bearer ' + token,
            '-d',
            json.dumps({'certificate_id': certificate_id}),
            opts['address'],
            'yandex.cloud.priv.certificatemanager.v1.CertificateContentService/Get',
        ]
    )
    if ret['retcode'] != 0:
        raise CommandExecutionError('get certificate content: {}'.format(ret['stderr']))
    return _parse_response(ret['stdout'])
