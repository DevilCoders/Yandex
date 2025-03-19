{% set mongodb = salt.slsutil.renderer('salt://components/mongodb/defaults.py?saltenv=' ~ saltenv) %}
{% set logdir = '/var/log/mdb-mongodb-perfdiag' %}

{# pushclient-related part #}
{{ logdir }}:
  file.directory:
    - user: monitor
    - group: monitor
    - watch_in:
      - service: dbaas-cron
      - service: pushclient

/etc/pushclient/mongodb-perfdiag.secret:
  file.managed:
{% if salt.pillar.get('data:perf_diag:lb_producer_key') %}
    - contents_pillar: data:perf_diag:lb_producer_key
{% elif salt.pillar.get('data:perf_diag:tvm_secret') %}
    - contents_pillar: data:perf_diag:tvm_secret
{% else %}
    - contents: 
      - "secret"
{% endif %}
    - makedirs: True
    - watch_in:
      - service: pushclient

/etc/pushclient/conf.d/topics-mongodb-perfdiag-grpc.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/topics-mongodb-perfdiag-grpc.conf
        - makedirs: True
        - watch_in:
            - service: pushclient

/etc/pushclient/pushclient-mongodb-perfdiag.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/pushclient-mongodb-perfdiag.conf
    - template: jinja
    - makedirs: True
    - require:
      - file: /etc/pushclient/mongodb-perfdiag.secret
      - file: {{ logdir }}
    - watch_in:
      - service: pushclient

/var/lib/push-client-perfdiag:
  file.directory:
    - user: statbox
    - group: statbox
    - makedirs: True
    - watch_in:
      - service: pushclient

{# dbaas-cron - related part #}
/etc/logrotate.d/mdb-mongodb-perfdiag:
  file.managed:
    - source: salt://{{ slspath }}/conf/logrotate

/etc/dbaas-cron/modules/mdb_mongodb_perfdiag_profiler.py:
  file.managed:
    - source: salt://{{ slspath }}/conf/mdb_mongodb_perfdiag_profiler.py
    - user: root
    - group: monitor
    - mode: 640
    - watch_in:
      - service: dbaas-cron

/etc/dbaas-cron/conf.d/mdb_mongodb_perfdiag_profiler.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/mdb_mongodb_perfdiag_profiler.conf
    - template: jinja
    - user: root
    - group: monitor
    - mode: 640
    - context:
        mongodb: {{mongodb | tojson}}
    - watch_in:
      - service: dbaas-cron
