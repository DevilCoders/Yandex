{% from slspath + '/map.jinja' import mongodb,cluster with context -%}

{% if mongodb.mongod %}
mongodb-alive:
  monrun.present:
    - type: mongodb
    - execution_interval: 300
    - command: /usr/bin/mongodb-check-alive mongodb
{% endif %}

{% if mongodb.keyFile and not mongodb.keyFile_nomanage %}
/etc/mongo-monitor.conf:
  file.managed:
    {% if mongodb.get("mongoMonitorPillar", False) %}
    - contents: {{ salt['pillar.get']('mongodb:mongoMonitorPillar') | json }}
    {% else %}
    - source:
      - salt://files/{{cluster}}/etc/mongo-monitor.conf
      - salt://{{cluster}}/etc/mongo-monitor.conf
    {% endif %}
    - mode: 0600
    - user: root
    - group: root
    - makedirs: True
{% endif %}

{% if mongodb.mongod or mongodb.mongodbcfg.mongod_monrun %}
{% with m = mongodb.monrun -%}
{% set primary = 'primary' if m["lock-on-primary-only"] else '' -%}
mongodb-asserts-user:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb asserts_user '{{m["asserts-user"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-connections-available:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb connections_available '{{m["connections-available"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-connections-current:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb connections_current '{{m["connections-current"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-disk-read:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb disk_read '{{m["disk-read"]}}' 10 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '30'
    - execution_timeout: '30'
    - type: mongodb
mongodb-disk-write:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb disk_write '{{m["disk-write"]}}' 10 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '30'
    - execution_timeout: '30'
    - type: mongodb
mongodb-flush-average-ms:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb backgroundFlushing_average_ms '{{m["flush-average-ms"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-fragmentation:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb fragmentation '{{m.fragmentation}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-globalLock-activeClients-total:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb globalLock_activeClients_total '{{m["activeClients-total"]}}' {{primary}} 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-globalLock-read-queue:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb globalLock_currentQueue_readers '{{m["read-queue"]}}' {{primary}} 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-globalLock-write-queue:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb globalLock_currentQueue_writers  '{{m["write-queue"]}}' {{primary}} 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-indexes-miss-ratio:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb indexCounters_btree_missRatio '{{m["indexes-miss-ratio"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-indexes-to-memory-ratio:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb indexes_to_memory_ratio '{{m["indexes-to-memory-ratio"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-resident-to-memory-ratio:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb resident_to_total_memory_ratio '{{m["resident-to-memory-ratio"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-uptime:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb uptime '{{m.uptime}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb
mongodb-recovering-state:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-recovery mongodb
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 300
    - type: mongodb
mongodb-pool:
  monrun.present:
    {% if mongodb.notArbiter -%}
    - command: /usr/bin/mongodb-check-pool mongodb
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 300
    - type: mongodb
mongodb-indexes-verification:
    monrun.present:
    {% if mongodb.notArbiter -%}
    - command: /usr/bin/mongodb-check-health mongodb verify_indexes '{{m.verify_indexes_errorcode}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 600
    - type: mongodb
mongodb-refill-from-hidden:
    monrun.present:
    {% if mongodb.notArbiter -%}
    - command: /usr/bin/mongodb-refill-from-hidden-check.sh 0 18000 /var/lib/dbmanager/mongo-refill-from-hidden
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 600
    - type: mongodb

{% set cs = m["mongodb_slow_queries_show_collscan"] %}
{% if cs == True %}
    {% set collscan = "--collscan-limit 0" %}
    {% if not mongodb.python3 -%}
       {% set collscan = "-c" -%}
    {%- endif %}
{% elif cs == False %}
    {% set collscan = "" %}
{% else %}
    {% set collscan = "--collscan-limit " + cs %}
{% endif %}

mongodb-slow-queries:
{% if mongodb.python3 %}
    pkg.installed:
      - name: yandex-media-mongo-slow-queries
{% else %}
    file.managed:
      - name: /usr/sbin/mongodb-slow-queries-v3.sh
      - source: salt://{{ slspath }}/files/usr/sbin/mongodb-slow-queries-v3.sh
      - user: root
      - group: root
      - mode: 755
{% endif %}
    monrun.present:
    {% if mongodb.monrun.mongodb_slow_queries_disabled -%}
    - command: echo '0;Check disabled. Pillar mongodb.monrun.mongodb_slow_queries_disabled'
    {% elif mongodb.notArbiter -%}
        {% if mongodb.python3 -%}
    - command: /usr/bin/mongodb-slow-queries.py {{collscan}} --slow-limit {{m.mongodb_slow_queries_count}} --time {{m.mongodb_slow_queries_ms}}
        {%- else %}
    - command: /usr/sbin/mongodb-slow-queries-v3.sh {{collscan}} -l '{{m.mongodb_slow_queries_count}}' -t '{{m.mongodb_slow_queries_ms}}' -p '{{m.mongodb_slow_queries_port}}'
        {%- endif %}
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 300
    - type: mongodb

/usr/sbin/mongodb-restore-state.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True
    - source: salt://{{ slspath }}/files/usr/sbin/mongodb-restore-state.sh

mongodb-restore:
  monrun.present:
    {% if mongodb.restore.monitor -%}
    - command: /usr/sbin/mongodb-restore-state.sh {{m.mongodb_restore_timedelta}}
    {%- else %}
    - command: echo '0; Check disabled in pillar'
    {%- endif %}
    - execution_interval: 600
    - type: mongodb

{% endwith %}
{%- endif %}

{% if mongodb.python3 -%}
# clean up old file
/usr/sbin/mongodb-slow-queries-v4.py:
  file.absent
{% endif %}
