include:
  - templates.mongodb.common
  - templates.mongodb.grants
  - common.config-caching-dns
  - common.s3cmd
{% if grains['yandex-environment'] != 'development' and grains['yandex-environment'] != 'stress' and grains['yandex-environment'] != 'unstable' %}
  - templates.mongodb.replicaset
{% endif %}
  - templates.push-client

/etc/distributed-flock-mongo-backup.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/distributed-flock-mongo-backup.json.{{ grains['yandex-environment'] }}
    - user: root
    - group: root
    - mode: 0644

/etc/mongo-backup.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/mongo-backup.conf.{{ grains['yandex-environment'] }}
    - template: jinja
    - user: root
    - group: root
    - mode: 0644

/etc/mongo-monitor.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/mongo-monitor.conf.{{ grains['yandex-environment'] }}
    - user: root
    - group: root
    - mode: 0600
