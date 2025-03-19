{% set mdb_hosts = ["man-55onbvjtzey57uom", "sas-ubdmgr4n6eovtt40", "vla-akdv659z12pp15mt"] %}
{% set mdb_port = 6432 %}
{% set pg_hosts = [] %}

{% for host in mdb_hosts -%}
  {% do pg_hosts.append("{}.db.yandex.net:{}".format(host, mdb_port)) %}
{% endfor %}

certificates:
  source: 'certs'
  contents:
    cmdb-test.yandex.net.key: {{ salt.yav.get('sec-01fj1tk89agdwzpne4k70gj29b[7F0018E908A08C885F15A9751800020018E908_private_key]') | json }}
    cmdb-test.yandex.net.pem: {{ salt.yav.get('sec-01fj1tk89agdwzpne4k70gj29b[7F0018E908A08C885F15A9751800020018E908_certificate]') | json }}

  packages: ["nginx"]
  services: "nginx"
  path: "/etc/nginx/ssl"

cmdb_domain: cmdb-test.yandex.net

cmdb_pgpassword: {{ salt.yav.get("sec-01fj1t70mv8xfa1n0h1fjpb86p[pgpassword]") | json }}
cmdb_tvm_secret: {{ salt.yav.get("sec-01fj1t70mv8xfa1n0h1fjpb86p[tvm_secret]") | json }}

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
    check_user_ticket: false
    check_service_ticket: false
    allowed_consumers_tvm_ids:
      - 2033613

  tvm:
    disk_cache_dir: /run/cmdb/cache/tvm/
    self_tvm_id: {{ salt.yav.get("sec-01fj1t70mv8xfa1n0h1fjpb86p[tvm_id]") | int | json }}
    enable_service_ticket_checking: true

  idm:
    allowed_consumers_tvm_ids:
      - 2001600 # IDM production

  rt:
    oauth_token: {{ salt.yav.get("sec-01fj1t70mv8xfa1n0h1fjpb86p[racktables_token]") | json }}
    endpoint: https://env-cffecaa0-fa4b-4c36-8d29-04273ee6c9bd.n5.test.racktables.yandex-team.ru/api
