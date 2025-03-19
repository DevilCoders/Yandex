yasmagent:
  instance-getter:
    {% for itype in ['storagesystem'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_collector a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}


redis:
  IPv6: '::'
  protected: 'no'
  tcp-backlog: 65535

{% set sentry_yav_record_id = 'ver-01ebknjhhbf7eev7wwzg9b7xga' -%}

sentry:
  db_name: sentry_mds
  db_user: {{ salt.yav.get(sentry_yav_record_id+'[pg.db-user]') }}
  db_password: {{ salt.yav.get(sentry_yav_record_id+'[pg.db-password]') }}
  secret-key: {{ salt.yav.get(sentry_yav_record_id+'[system.secret-key]') }}
  redis_host: sentry01vla.mds.yandex.net
  web-workers: 30
  tcp-backlog: 1024
