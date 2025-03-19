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
    m4s.yandex.net.key: {{ salt.yav.get('sec-01g88t0w91h69vk8s8z0mvgvrb[7F001DDF1E35BB3C93E050B1710002001DDF1E_private_key]') | json }}
    m4s.yandex.net.pem: {{ salt.yav.get('sec-01g88t0w91h69vk8s8z0mvgvrb[7F001DDF1E35BB3C93E050B1710002001DDF1E_certificate]') | json }}
  path      : "/etc/nginx/ssl/"
  packages  : ['nginx']
  services  : 'nginx'
