sync_mongod_databases:
{% if salt.pillar.get('data:mongodb:disable_db_management', False) != True %}
    mdb_mongodb.ensure_databases:
        - service: mongod
{% else %}
    test.nop
{% endif %}
