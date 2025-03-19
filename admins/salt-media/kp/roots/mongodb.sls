include:
  - templates.mongodb.common
  - templates.mongodb.replicaset

{% for instance in 'mongodb','mongos','mongodbcfg' %}
mongod_remove_default_{{ instance }}:
  file.absent:
    - name: /etc/{{ instance }}-default.conf
{% endfor %}

mongodbcfg_disabled:
  service.dead:
    - name: mongodbcfg

{% for group in ['jkp_db_mongo-prod-tasks-mongodb', 'jkp_db_mongo-prod-default-mongodb', 'jkp-test-db-mongo-tasks-mrs'] %}
{% if group in salt['grains.get']('c') %}
/etc/config-monrun-cpu-check/config.yml:
  yafile.managed:
    - makedirs: true
    - source: salt://files/etc/config-monrun-cpu-check/config.yml
{% endif %}
{% endfor %}
