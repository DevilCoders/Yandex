"""
Simple dns-resolver mock
"""

from types import SimpleNamespace

from dns.rdatatype import AAAA, A
from dns.resolver import NXDOMAIN

from .utils import handle_action


def query(state, fqdn, record_type):
    """
    Resolve mock
    """
    str_type = None
    if record_type == AAAA:
        str_type = 'AAAA'
    elif record_type == A:
        str_type = 'A'

    if not str_type:
        raise RuntimeError(f'Unsupported record type: {repr(record_type)}')

    action_id = f'dns-resolve-{fqdn}-{str_type}'
    handle_action(state, action_id)

    result = state['dns'].get(f'{fqdn}-{str_type}')

    if result:
        return SimpleNamespace(
            response=SimpleNamespace(answer=[SimpleNamespace(items=[SimpleNamespace(address=result)])])
        )

    raise NXDOMAIN()


def dns_resolver(mocker, state):
    """
    Setup dns-resolver mock
    """
    resolver = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.dns.slayer.Resolver')
    resolver.return_value.query.side_effect = lambda fqdn, record_type: query(state, fqdn, record_type)
