{% set yaenv = grains['yandex-environment'] %}

include:
  - templates.mongodb.common
  - templates.mongodb.replicaset
  - templates.mongodb.grants

{% for instance in 'mongodb','mongos','mongodbcfg' %}
/etc/{{ instance }}-default.conf:
  file.absent
{% endfor %}

mongodbcfg_disabled:
  service.dead:
    - name: mongodbcfg

{% if salt['grains.get']('conductor:group') in ['kp-kino-mongo-mrs'] %}
/etc/config-monrun-cpu-check/config.yml:
  yafile.managed:
    - makedirs: true
    - source: salt://files/etc/config-monrun-cpu-check/config.yml
{% endif %}
