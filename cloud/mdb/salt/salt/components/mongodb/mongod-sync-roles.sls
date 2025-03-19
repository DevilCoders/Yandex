{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

sync_mongod_roles:
    mdb_mongodb.ensure_roles:
        - roles: {{ mongodb.roles }}
        - userdb_roles: {{ mongodb.userdb_roles }}
        - service: mongod
