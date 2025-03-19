{% set mongodb = salt.slsutil.renderer('salt://components/mongodb/defaults.py?saltenv=' ~ saltenv) %}
{% set logdir = '/var/log/mdb-mongodb-perfdiag' %}

{# pushclient-related part #}
{{ logdir }}:
  file.absent

/etc/pushclient/mongodb-perfdiag.secret:
{% if salt.pillar.get('data:ship_logs', False) %}
  file.absent:
    - watch_in:
      - service: pushclient
{% else %}
  file.absent
{% endif %}

/etc/pushclient/conf.d/topics-mongodb-perfdiag-grpc.conf:
{% if salt.pillar.get('data:ship_logs', False) %}
  file.absent:
    - watch_in:
      - service: pushclient
{% else %}
  file.absent
{% endif %}

/etc/pushclient/pushclient-mongodb-perfdiag.conf:
{% if salt.pillar.get('data:ship_logs', False) %}
  file.absent:
    - watch_in:
      - service: pushclient
{% else %}
  file.absent
{% endif %}

{# dbaas-cron - related part #}
/etc/dbaas-cron/modules/mdb_mongodb_perfdiag_profiler.py:
  file.absent:
    - watch_in:
      - service: dbaas-cron

/etc/dbaas-cron/conf.d/mdb_mongodb_perfdiag_profiler.conf:
  file.absent:
    - watch_in:
      - service: dbaas-cron
