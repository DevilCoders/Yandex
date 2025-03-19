{% set env = grains['yandex-environment'] %}
{% if env == 'production' %}
  {% set sec_id = 'sec-01d5xbwc006n6c27dm4n3b0f25' %}
  {% set zk_list = ["kp-zk01e.kp.yandex.net:2181", "kp-zk01j.kp.yandex.net:2181", "kp-zk01h.kp.yandex.net:2181"] %}
  {% set bucket = 'jkp-db-production' %}
{% else %}
  {% set sec_id = 'sec-01d5xbvwbd90eh64edwvxdb32x' %}
  {% set zk_list = ["kp-zk01i.tst.kp.yandex.net:2181", "kp-zk01f.tst.kp.yandex.net:2181", "kp-zk01h.tst.kp.yandex.net:2181"] %}
  {% set bucket = 'jkp-db-testing' %}
{% endif %}
---
mongodb:
  mongo_hack: False
  backup:
    standalone: 2
    rsync:
      realm: 'mongo-backup/jkp'
      server: 'mongo-backup.jkp.yandex.net'
    mongo_user: 'backup'
    mongo_password: {{ salt.yav.get(sec_id + '[backup]') | json }}
    zk:
      hosts: {{ zk_list }}
    transport: 's3cmd'
    bucket: {{ bucket }}
    compressor: 'zstd'
    mongos_host: ''
