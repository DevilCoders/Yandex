{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

{% if salt.mdb_mongodb_helpers.deploy_service('mongod') or (salt.mdb_mongodb_helpers.deploy_service('mongocfg') and not salt.mdb_mongodb_helpers.is_replica('mongocfg')) %}
include:
{% endif %}
{% if salt.mdb_mongodb_helpers.deploy_service('mongod') %}
    - .mongod-set-oplog-maxsize
{% endif %}
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%     if not salt.mdb_mongodb_helpers.is_replica(srv) and salt.mdb_mongodb_helpers.deploy_service(srv) %}
    - .{{ srv }}-set-feature-compatibility-version
{%     endif %}
{% endfor %}

{% if salt.mdb_mongodb_helpers.deploy_service('mongod') or (salt.mdb_mongodb_helpers.deploy_service('mongocfg') and not salt.mdb_mongodb_helpers.is_replica('mongocfg')) %}
extend:
{% endif %}
{% if salt.mdb_mongodb_helpers.deploy_service('mongod') %}
    mongodb-set-oplog-maxsize:
        mdb_mongodb.oplog_maxsize:
            - require:
                - mongod-ready-for-user
{% endif %}
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%     if not salt.mdb_mongodb_helpers.is_replica(srv) and salt.mdb_mongodb_helpers.deploy_service(srv) %}
    {{srv}}-set-feature-compatibility-version:
        mongodb.feature_compatibility_version:
            - require:
                - {{srv}}-ready-for-user
{%     endif %}
{% endfor %}
