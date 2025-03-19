{% set mdb_hosts = ["man-2gz9t2m7yob2m7bo", "sas-cq3h8t2hcpxgc0ck", "vla-9jfi0o174hmycve8"] %}
{% set mdb_port = 6432 %}
{% set pg_hosts = [] %}

{% for host in mdb_hosts -%}
  {% do pg_hosts.append("{}.db.yandex.net:{}".format(host, mdb_port)) %}
{% endfor %}

certificates:
  source: 'certs'
  contents:
    cmdb.yandex.net.key: {{ salt.yav.get('sec-01fhtej6dpbkavr3xmdamxeh1m[7F0018DAEC22101FF3B0C49FE000020018DAEC_private_key]') | json }}
    cmdb.yandex.net.pem: {{ salt.yav.get('sec-01fhtej6dpbkavr3xmdamxeh1m[7F0018DAEC22101FF3B0C49FE000020018DAEC_certificate]') | json }}

  packages: ["nginx"]
  services: "nginx"
  path: "/etc/nginx/ssl"


cmdb_domain: cmdb.yandex.net

cmdb_pgpassword: {{ salt.yav.get("sec-01fegv39dah61c2s9e58z39nqr[pgpassword]") | json }}
cmdb_tvm_secret: {{ salt.yav.get("sec-01fegv39dah61c2s9e58z39nqr[tvm_secret]") | json }}

cmdb_config:
  logger:
    level: info
    sinks:
      - logrotate:///var/log/cmdb/cmdb.log

  http:
    port: 8088
    shutdown_timeout: 15s

  database:
    uri: postgresql://cmdb@{{ pg_hosts | join(",") }}?dbname=cmdb&sslmode=verify-full
    uri_rw: postgresql://cmdb@{{ pg_hosts | join(",") }}?dbname=cmdb&sslmode=verify-full&target_session_attrs=read-write

  auth:
    check_user_ticket: true
    check_service_ticket: true
    allowed_consumers_tvm_ids:
      - 2033629
      - 2030827

  tvm:
    disk_cache_dir: /run/cmdb/cache/tvm/
    self_tvm_id: {{ salt.yav.get("sec-01fegv39dah61c2s9e58z39nqr[tvm_id]") | int | json }}
    enable_service_ticket_checking: yes
    enable_user_ticket_checking: true

  idm:
    allowed_consumers_tvm_ids:
      - 2001600 # IDM production

  wiki:
    cmdb_host: cmdb.yandex-team.ru
