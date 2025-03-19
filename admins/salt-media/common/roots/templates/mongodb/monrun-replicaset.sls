{% from slspath + '/map.jinja' import mongodb,cluster with context %}

mongodb-master-changed:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb is_master diff 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb

mongodb-health:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-aggr-health
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb

{% with m = mongodb.monrun %}
mongodb-master-present:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb master_present '{{m["master-present"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '60'
    - type: mongodb

mongodb-rs-indexes-consistency:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb rs_indexes_consistency '{{m["rs-indexes-consistency"]}}' 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: '600'
    - type: mongodb

mongodb-replica-lag:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-rs mongodb {{ m["replica-lag-hidden"] if mongodb.isHidden else m["replica-lag-common"] }}
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 300
    - type: mongodb

mongodb-rs-secondary-alive:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb rs_secondary_alive {{ m["rs_secondary_alive_count"] }} 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 60
    - type: mongodb

mongodb-rs-secondary-fresh:
  monrun.present:
    {%- if mongodb.notArbiter %}
    - command: /usr/bin/mongodb-check-health mongodb rs_secondary_fresh {{ m["rs_secondary_fresh_count"] }} {{ m["rs_secondary_fresh_lag"] }} 2> /dev/null
    {%- else %}
    - command: echo '0;N/A;this is arbiter host'
    {%- endif %}
    - execution_interval: 60
    - type: mongodb


{% endwith %}
