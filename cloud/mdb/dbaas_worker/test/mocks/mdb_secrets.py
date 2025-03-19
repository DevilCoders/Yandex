"""
Simple MDB Secrets mock
"""

from httmock import urlmatch

from .utils import handle_http_action, http_return


def get_query_params(query):
    """
    Get query params
    """
    params = {}
    for pair in query.split('&'):
        key, value = pair.split('=')
        params[key] = value

    return params


def mdb_secrets(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='mdb_secrets.test', method='put', path='/v1/cert')
    def get_or_issue_cert(url, request):
        params = get_query_params(url.query)
        fqdn = params['hostname']
        ret = handle_http_action(state, f'mdb_secrets-issue-{fqdn}')
        if ret:
            return ret

        new_cert = None
        if fqdn in state['mdb_secrets']:
            use_state = state['mdb_secrets'][fqdn]
            if params.get('alt-names', []) != use_state['alt-names'] or params['ca'] != use_state['ca']:
                new_cert = f'reissue-cert-for-{fqdn}'
        else:
            new_cert = f'cert-for-{fqdn}'

        if new_cert:
            state['mdb_secrets'][fqdn] = {
                'ca': params['ca'],
                'alt-names': params.get('alt-names', []),
                'ekey': f'encrypted-key-for-{fqdn}',
                'cert': new_cert,
            }

        ret_body = {
            'key': {'version': 1, 'data': state['mdb_secrets'][fqdn]['ekey']},
            'cert': state['mdb_secrets'][fqdn]['cert'],
        }

        return http_return(code=200, body=ret_body)

    return (get_or_issue_cert,)
