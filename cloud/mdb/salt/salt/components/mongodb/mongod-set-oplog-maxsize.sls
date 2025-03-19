{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set config = mongodb.config.get('mongod') %}

mongodb-set-oplog-maxsize:
    mdb_mongodb.oplog_maxsize:
        - max_size: {{ config._settings.oplogMaxSize }}
        - service: mongod
