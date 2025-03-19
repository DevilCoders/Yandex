{% set zk_hosts = salt['conductor']['groups2hosts']('elliptics-zk') | map('regex_replace', '$', ':2181') | list %}
{% set mongo_hosts = salt['conductor']['groups2hosts']('ape-mongo') | list %}
cluster : elliptics-logshatter

include:
  - units.push-client.logshatter

yasmagent:
    instance-getter:
      {% for itype in ['apelogshatter'] %}
      - echo {{ grains['conductor']['fqdn'] }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}

logshatter_zookeper: {{ zk_hosts | yaml }}
logshatter_mongo: {{ mongo_hosts | yaml }}
logshatter_max_partitions: {{ grains['num_cpus'] * 4}}
logshatter_read_threads: {{ grains['num_cpus'] }}
logshatter_parse_threads: {{ grains['num_cpus'] * 2 }}
logshatter_max_output_threads: {{ grains['num_cpus'] / 16 | int }}

logshatter:
  tvm:
    client_ids:
      logbroker: 2001059
      logshatter: 2013448
    secret: {{ salt.yav.get('ver-01f3zehaj8djq1tkm5ye00ebjn[logshatter_tvm_secret]') }}

  clickhouse:
    user: {{ salt.yav.get('ver-01f3zehaj8djq1tkm5ye00ebjn[logshatter_clickhouse_user]') }}
    password: {{ salt.yav.get('ver-01f3zehaj8djq1tkm5ye00ebjn[logshatter_clickhouse_password]') }}

clickphite:
  user: {{ salt.yav.get('ver-01f3zgzzghbweq686cw5k67s2d[clickphite_robot_user]') }}
  password: {{ salt.yav.get('ver-01f3zgzzghbweq686cw5k67s2d[clickphite_robot_password]') }}

  grafana_token: {{ salt.yav.get('ver-01f3zgzzghbweq686cw5k67s2d[clickphite_grafana_token]') }}
  solomon_token: {{ salt.yav.get('ver-01f3zgzzghbweq686cw5k67s2d[clickphite_solomon_token]') }}

  clickhouse:
    user: {{ salt.yav.get('ver-01f3zgzzghbweq686cw5k67s2d[logshatter_clickhouse_user]') }}
    password: {{ salt.yav.get('ver-01f3zgzzghbweq686cw5k67s2d[logshatter_clickhouse_password]') }}
