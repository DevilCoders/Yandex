"""
Simple certificator mock
"""

import json

from httmock import urlmatch

from .utils import handle_http_action, http_return


CERT = '''-----BEGIN CERTIFICATE-----
MIIF0jCCA7oCAQAwDQYJKoZIhvcNAQENBQAwga4xCzAJBgNVBAYTAk5UMRwwGgYD
VQQIDBNzdGF0ZU9yUHJvdmluY2VOYW1lMRUwEwYDVQQHDAxsb2NhbGl0eU5hbWUx
GTAXBgNVBAoMEG9yZ2FuaXphdGlvbk5hbWUxHTAbBgNVBAsMFG9yZ2FuaXphdGlv
blVuaXROYW1lMRMwEQYDVQQDDApjb21tb25OYW1lMRswGQYJKoZIhvcNAQkBFgxl
bWFpbEFkZHJlc3MwHhcNMjEwNTMxMTYwNjI3WhcNMzEwNTI5MTYwNjI3WjCBrjEL
MAkGA1UEBhMCTlQxHDAaBgNVBAgME3N0YXRlT3JQcm92aW5jZU5hbWUxFTATBgNV
BAcMDGxvY2FsaXR5TmFtZTEZMBcGA1UECgwQb3JnYW5pemF0aW9uTmFtZTEdMBsG
A1UECwwUb3JnYW5pemF0aW9uVW5pdE5hbWUxEzARBgNVBAMMCmNvbW1vbk5hbWUx
GzAZBgkqhkiG9w0BCQEWDGVtYWlsQWRkcmVzczCCAiIwDQYJKoZIhvcNAQEBBQAD
ggIPADCCAgoCggIBALu/Us7WKoCWnXhLvHKVkJ7Kul2F+8XMdhqraO7K7HX7gBmQ
P8h0ChsKCZK5N9RwqqF71mCIkV5Rul2aP++qZQF9V/KEYYUU4y/b+xgZUQ5Rs/q1
aa8++JtV4oPR1tyPq5IGsdMPnbKWo484Jw+8utoh7tPfaPrMNYQ1TOsoB6wN3HUe
bky1aBSS3d0OFk6kt0XNlW464zmooKIrnA4D+cW0cRwHOS+3cE/u0SYypOaW8NBS
EiZzcPOb7cIvnTp4a7ss+LdRZY4gc3t39YgbRsQzOBMlMTARWUWi+1YkPK6eUpRd
0AlPJE/11k+icrQsdvfDA0rpbG3+ZddWmQypfPFX8j7Z/vgl5uWrBxbAXGRPdWsV
+FksAjNQ/Gy6xbQ4Cf6V79l4hNF+6ThpH2lBktG5rlRlzSqUs/AmxpzCnxg+nn+0
4RhY7h74e8Zq4Z886iSnJY3/W20bzqOD67FDvwK5msNLF1aDQSkwvI1mby6tMa6e
jkHRjwfWamkxWs+A7g9G7XCjjUxB95efJXDtvOdh9tS/GLPNTB4FBX/bJ6uGQLT6
dBk55MV2JbGrtvSiS5phRZmk7eaIw/4EPomwW0QPaRhxOmPq4I4+RWRp092lAMtR
ifJ8P3ipEzRjZdotiPskh2fv0v+rcc0rl5tqQDdKl3ExJzrdOjxR7kQQKentAgMB
AAEwDQYJKoZIhvcNAQENBQADggIBACjAmH9KQR9U+cm+VCx4UWgFFjIBaFmfSBxX
uy+kINtzm9jWDoCvxsT8xJx96Ly3zuHQN8hIuzwUGekgZjiPdveN3IebV85AEr3I
wWlPTrZpkKwNIFrycwPIncWqNDEQygFS3KSFTGmreVZ380G7Y5AlUSrwehA5J4ng
vHv7nSeXkX7Iw2p3X2J8P8axjmgPbH5xYV1grpXg/LRZwebMfMCiLXoMsRZoBhUO
i9gl5+16J7spcWPbcaNUmgK2qSShhfeMKOCwP+EP0o1lWKE9JAL8m2F8QDq8XOiZ
/4vaxqpGnZq8R9dgu0AlRR6wMOCmOvIQm6lhoYr3K6VtlOBxBdx01ZYXQJNypszO
ZLhW5k5BtMAIJiETRd69Yxaq5DkQpNW6SKftnvckihY6YhiErzxKrGiiLoVIcYrt
G/IEkILUEXq7IkrFq+0O9Sn2THFz3uiUC9zxUxbpflhIJ9GK7GG7s330gQHCgxjj
bA1bkrXv0wSbMhyHIbVeikWGzRH07gmXR8N0Da6lh/va0sf4/FDcpvxVu28HtWD3
MlS6kUOLBE75HMUdN4snPxU0bgrvt+YPHZRJPmI7rSbzjxc/LJOVuMp9aSnybEmW
PP5Mo87jPcqnkKfCOYqSNe++TISSMJL/9IVbzFGSTi/P2B8CAfMFCPeLi3AajffD
kNn5Ukif
-----END CERTIFICATE-----'''


def format_cert(cert):
    """
    Format cert to fool worker fsm
    """
    return f'--BEGIN PRIVATE KEY--\ndummy\n--END PRIVATE KEY--\n{cert}'


def certificator(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='certificator.test', method='get', path='/api/certificate/')
    def get_cert(url, _):
        fqdn = url.query.split('=')[1]
        ret = handle_http_action(state, f'certificator-get-{fqdn}')
        if ret:
            return ret

        if fqdn in state['certificator']:
            return http_return(
                body={
                    'count': 1,
                    'results': [
                        {
                            'status': 'issued',
                            'revoked': None,
                            'issued': '1970-01-01T00:00:01Z',
                            'end_date': '3000-01-01T00:00:00Z',
                            'hosts': state['certificator'][fqdn]['hosts'],
                            'url': f'http://certificator.test/api/certificate/{fqdn}',
                            'download2': f'http://certificator.test/download/{fqdn}',
                        }
                    ],
                }
            )

        return http_return(body={'count': 0})

    @urlmatch(netloc='certificator.test', method='post', path='/api/certificate/')
    def issue_cert(_, request):
        data = json.loads(request.body)
        fqdn = data['hosts'].split(',')[0]
        ret = handle_http_action(state, f'certificator-issue-{fqdn}')
        if ret:
            return ret

        state['certificator'][fqdn] = {
            'hosts': data['hosts'].split(','),
            'cert': CERT,
        }

        return http_return(code=201, body={'download2': f'http://certificator.test/download/{fqdn}'})

    @urlmatch(netloc='certificator.test', method='delete', path='/api/certificate/.+')
    def revoke_cert(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'certificator-issue-{fqdn}')
        if ret:
            return ret

        if fqdn not in state['certificator']:
            return http_return(code=500, body=f'Cert for {fqdn} not issued')

        del state['certificator'][fqdn]

        return http_return(body={'revoked': '1970-01-01T00:00:00Z'})

    @urlmatch(netloc='certificator.test', method='get', path='/download/.+')
    def download_cert(url, _):
        fqdn = url.path.split('/')[-1].replace('.pem', '')
        ret = handle_http_action(state, f'certificator-download-{fqdn}')
        if ret:
            return ret

        if fqdn not in state['certificator']:
            return http_return(code=404, body=f'Cert for {fqdn} not issued')

        return http_return(body=format_cert(state['certificator'][fqdn]['cert']))

    return (get_cert, issue_cert, revoke_cert, download_cert)
