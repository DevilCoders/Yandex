{% set mdb_hosts = ["man-63kpvl42uf72q5q3", "sas-7roe4ppfto43p9hb", "vla-fr6f96l3yqa2j7zf"] %}
{% set mdb_port = 6432 %}
{% set pg_hosts = [] %}

{% for host in mdb_hosts -%}
  {% do pg_hosts.append("{}.db.yandex.net:{}".format(host, mdb_port)) %}
{% endfor %}

pg_hosts: {{ pg_hosts | join(",") }}
secrets: {{ salt.yav.get("sec-01ffpyv7xxyq56t2zx8em2fhpg") | json }}

certificates:
  contents  :
    macros.tst.yandex-team.ru.key: {{ salt.yav.get('sec-01ffsy1p49j2krzf4marp0t7cr[7F00186C1AD446E69017D3A566000200186C1A_private_key]') | json }}
    macros.tst.yandex-team.ru.pem: {{ salt.yav.get('sec-01ffsy1p49j2krzf4marp0t7cr[7F00186C1AD446E69017D3A566000200186C1A_certificate]') | json }}
  path      : "/etc/nginx/ssl/"
  packages  : ['nginx']
  services  : 'nginx'
