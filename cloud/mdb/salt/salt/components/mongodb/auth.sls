{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

{% for srv in salt.mdb_mongodb_helpers.services_deployed() if salt.mdb_mongodb_helpers.deploy_service(srv) %}
{%  if not salt.mdb_mongodb_helpers.is_replica(srv) %}
{%   set config = mongodb.config.get(srv) %}
add-admin-user-{{srv}}:
    mongodb.user_create:
        - name: admin
        - passwd: {{ salt.pillar.get('data:mongodb:users:admin:password')|yaml_encode }}
        - roles: ['root']
        - authdb: admin
        - host: localhost
        - port: {{config.net.port}}
        - unless:
            - "{{config.cli.cmd}} --quiet --norc --username admin --password {{ salt.pillar.get('data:mongodb:users:admin:password') }} admin --eval 'quit()'"
        - require:
            - mdb_mongodb: {{srv}}-service
            - service: {{srv}}-service
{%   if srv in ('mongod', 'mongocfg') %}
            - mongodb: {{srv}}-wait-for-primary-state
{%   endif %}
{%  endif %}

wait-for-admin-user-{{srv}}:
    mdb_mongodb.wait_user_created:
      - user: admin
      - service: {{srv}}
      - timeout: 300
      - require:
        - mdb_mongodb: {{srv}}-service
        - service: {{srv}}-service
{%   if not salt.mdb_mongodb_helpers.is_replica(srv) %}
        - mongodb: add-admin-user-{{srv}}
{%   else %}
        - {{srv}}-replicaset-member  
{%   endif %}

{% set mdb_mongo_tools_available = salt.pillar.get('data:mdb_mongo_tools:enabled', True) and (mongodb.use_mongod or mongodb.use_mongocfg) %}
wait-for-resetup-if-any-{{srv}}:
{% if mdb_mongo_tools_available %}
    mdb_mongodb.wait_for_resetup:
      - service: {{srv}}
{% else %}
    test.nop:
{% endif %}
      - require:
        - mdb_mongodb: {{srv}}-service
        - service: {{srv}}-service
        - wait-for-admin-user-{{srv}}
{% if mdb_mongo_tools_available %}
        - mdb-mongo-tools-ready
{% endif %}

{# State for require, says that mongo(d|cfg|s) is ready for user,
  i.e up, in right state and has admin user (and may be something more)#}
{{srv}}-ready-for-user:
    test.nop:
      - require:
        - mdb_mongodb: {{srv}}-service
        - service: {{srv}}-service
        - wait-for-admin-user-{{srv}}
        - wait-for-resetup-if-any-{{srv}}

{% endfor %}

{% if ((salt.mdb_mongodb_helpers.deploy_service('mongod') and not salt.mdb_mongodb_helpers.is_replica('mongod'))
     or salt.mdb_mongodb_helpers.deploy_service('mongos')) %}
include:
{%   for srv in salt.mdb_mongodb_helpers.services_deployed(['mongod', 'mongos']) if (
                                              salt.mdb_mongodb_helpers.deploy_service(srv)
                                              and not salt.mdb_mongodb_helpers.is_replica(srv)) %}
    - .{{ srv }}-sync-roles
    - .{{ srv }}-sync-users
    - .{{ srv }}-sync-databases
{%   endfor %}

extend:
{%   for srv in salt.mdb_mongodb_helpers.services_deployed(['mongod', 'mongos']) if (
                                              salt.mdb_mongodb_helpers.deploy_service(srv)
                                              and not salt.mdb_mongodb_helpers.is_replica(srv)) %}
    sync_{{ srv }}_roles:
      mdb_mongodb.ensure_roles:
        - require:
          - {{srv}}-ready-for-user


    sync_{{ srv }}_users:
      mdb_mongodb.ensure_users:
        - require:
          - mdb_mongodb: sync_{{srv}}_roles

    sync_{{ srv }}_databases:
      mdb_mongodb.ensure_databases:
        - require:
          - mdb_mongodb: sync_{{srv}}_users
        - require_in:
          - {{srv}}-total-done
{%   endfor %}
{% endif %}
