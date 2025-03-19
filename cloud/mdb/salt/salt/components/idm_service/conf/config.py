{% set metadb = salt['pillar.get']('data:config:metadb') %}
{% set hosts = metadb['hosts'] %}
{% set port = metadb['port'] %}
{% set dbname = metadb['dbname'] %}
{% set user = metadb['user'] %}

TVM_ENABLED = {{ salt['pillar.get']('data:tvm:enabled', True) | python }}
CLIENT_ID = {{ salt['pillar.get']('data:tvm:client_id', '') | python }}
CLIENT_SECRET = {{ salt['pillar.get']('data:tvm:client_secret', '') | python }}
ENV = {{ salt['pillar.get']('data:tvm:env', 'prod') | python }}
IDM_CLIENTS = {
    'idm': {
        'test': 2001602,
        'prod': 2001600,
    },
    'soc': {
        'test': 2001602,
        'prod': 2001600,  # TODO: change me
    }
}

CONNSTRINGS = [
    {% for host in hosts %}
    'host={{ host }} port={{ port }} dbname={{ dbname }} user={{ user }} connect_timeout=1 tcp_user_timeout=1 keepalives_idle=2 keepalives_interval=1 keepalives_count=3',
    {% endfor %}
]

CRYPTO = {{ salt['pillar.get']('data:config:crypto') }}

STORE_PASSWORD_IN_VAULT = {{ salt['pillar.get']('data:yav:enabled', True) | python }}
VAULT_OAUTH_TOKEN = {{ salt['pillar.get']('data:yav:token') | python }}
