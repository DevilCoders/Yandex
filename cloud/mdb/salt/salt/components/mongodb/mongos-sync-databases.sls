sync_mongos_databases:
{% if salt.pillar.get('data:mongodb:disable_db_management', False) != True %}
    mdb_mongodb.ensure_databases:
        - service: mongos
{% else %}
    test.nop
{% endif %}
