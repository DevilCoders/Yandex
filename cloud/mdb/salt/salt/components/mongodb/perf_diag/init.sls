{% set mongodb = salt.slsutil.renderer('salt://components/mongodb/defaults.py?saltenv=' ~ saltenv) %}

include:
  {% if salt.pillar.get('data:perf_diag:mongodb:enabled', False) and (mongodb.use_mongod) %}
  {# Perfdiag can be used on mongod only (it is useless on mongocfg & can't work (yet?) on mongos instances) #}

  - .profiler
  {% else %}
  - .disable-profiler
  {% endif %}

{#
perfdiag pillar:
  data:
    perf_diag:
      tvm_client_id: ....
      tvm_server_id: ....
      tvm_secret: ....
      topics:
        mongodb-profiler: /mdb/porto/prod/perf_diag/mongodb_profiler
      mongodb:
        enabled: False
        interval: 5
        use_all_databases: True
        databases: []
        exclude_databases:
          - local
          - config
          - admin
          - mdb_internal
        user: monitor

#}
