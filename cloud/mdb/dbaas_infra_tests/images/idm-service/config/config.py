"""
IDM Service config.
"""

TVM_ENABLED = False
STORE_PASSWORD_IN_VAULT = False

CONNSTRINGS = [
    'host=metadb0%s.{{ conf.network_name }} '
    'port=5432 user={{ conf.projects.metadb.db.user }} '
    'password={{ conf.projects.metadb.db.password }} '
    'dbname={{ conf.projects.metadb.db.dbname }}' % x for x in (1, 2, 3)
]

CRYPTO = {
    'api_sec_key': '{{ conf.dynamic.internal_api.pki.secret }}',
    'client_pub_key': '{{ conf.dynamic.salt.pki.public }}',
}

SMTP_HOST = 'fake_smtp01.{{ conf.network_name }}:5000'
