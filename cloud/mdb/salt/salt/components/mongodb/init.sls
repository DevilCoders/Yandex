{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

include:
{% if salt.pillar.get('data:perf_diag', False) %}
{# Manage perfdiag states only if perfdiag is present in current installation#}

    - .perf_diag
{% endif %}
    - .directories
    - components.monrun2.mongodb
    - components.logrotate
{% if salt.pillar.get('data:mdb_mongo_tools:enabled', True) and (mongodb.use_mongod or mongodb.use_mongocfg) %}
    - .tools
{% elif salt.pillar.get('data:mdb_mongo_tools:install', True) %}
    - .tools.pkgs
{% endif %}
{% if salt.mdb_mongodb_helpers.deploy_service('mongod') or salt.mdb_mongodb_helpers.deploy_service('mongocfg') %}
    - .mongod
    - .limits
    - .sysctl
    - .groups
{% endif %}
    - .settings
{% if mongodb.use_auth  %}
    - .ssl
    - .auth
{% endif %}
{% if salt.mdb_mongodb_helpers.deploy_service('mongos') %}
    - .mongos
{% endif %}
{% if mongodb.use_wd_mongodb %}
    - .wd-service
{% endif %}
{% if salt.pillar.get('data:mdb_metrics:enabled', False) %}
    - .mdb-metrics
    - .yasmagent
{% else %}
    - components.mdb-metrics.disable
{% endif %}
    - .common
    - .walg.pkgs
{% if (salt.mdb_mongodb_helpers.deploy_service('mongod') or salt.mdb_mongodb_helpers.deploy_service('mongocfg')) and salt.pillar.get('data:walg:enabled', False)  %}
    - .walg
{% endif %}
{% if (salt.mdb_mongodb_helpers.deploy_service('mongod') or salt.mdb_mongodb_helpers.deploy_service('mongocfg')) %}
    - .encryption
{% endif %}
{% if salt.pillar.get('data:ship_logs', False) %}
    - components.pushclient2
    - .pushclient
{% endif %}
{% if salt.pillar.get('service-restart', False) %}
    - .restart
{% endif %}
{% if salt.pillar.get('service-stepdown', False) %}
    - .stepdown
{% endif %}
{% if salt.pillar.get('data:dbaas:cluster') %}
    - .resize
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.firewall
    - components.firewall.external_access_config
    - .firewall
{% endif %}
{% endif %}

mongodb-total-req:
  test.nop

mongodb-total-done:
  test.nop

extend:
{# We do this via extend so we'll have extend section unconditionally #}
  mongodb-total-req:
    test.nop:
      - require_in:
{% for srv in salt.mdb_mongodb_helpers.services_deployed() if salt.mdb_mongodb_helpers.deploy_service(srv) %}
        - test: {{srv}}-total-req
{% endfor %}

  mongodb-total-done:
    test.nop:
      - require:
{% for srv in salt.mdb_mongodb_helpers.services_deployed() if salt.mdb_mongodb_helpers.deploy_service(srv) %}
        - test: {{srv}}-total-done
{% endfor %}

{% if salt.mdb_mongodb_helpers.deploy_service('mongos') and salt.mdb_mongodb_helpers.deploy_service('mongocfg') %}
{%   if salt.pillar.get('mongos-first', False) %}
  mongocfg-total-req:
    test.nop:
      - require:
        - test: mongos-total-done
{%   else %}
  mongos-total-req:
    test.nop:
      - require:
        - test: mongocfg-total-done
{%   endif %}
{% endif %}

{# In case of restart + stepdown, do stepdown first and then restart, do it only for services which will be restarted (-mongos) #}
{% if salt.pillar.get('service-restart', False) and salt.pillar.get('service-stepdown', False)  %}
{%    set service_for_restart = salt.pillar.get('service-for-restart', None) %}
{%    for srv in salt.mdb_mongodb_helpers.services_deployed() if srv != 'mongos' and service_for_restart in [srv, None] %}
  {{srv}}-restart-prereq:
    test.nop:
      - require:
        - mdb_mongodb: stepdown_{{srv}}_host
{%    endfor %}
{% endif %}
