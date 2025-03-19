{% set do_restart = salt.pillar.get('service-restart') and salt.pillar.get('service-for-restart', None) in ['mongod', None] %}
include:
    - components.mongodb.mongod-set-feature-compatibility-version
    - components.mongodb.mongod-config
    - components.mongodb.mongod-service
{% if do_restart %}
    - components.mongodb.mongod-restart
{% endif %}
{% if salt.pillar.get('service-stepdown') %}
    - components.mongodb.mongod-stepdown-host
{% endif %}

extend:
    mongod-service:
        service.running:
            - watch:
                - file: /etc/mongodb/mongodb.conf
{% if do_restart and salt.pillar.get('service-stepdown') %}
    mongod-restart-prereq:
      test.nop:
        - require:
          - mdb_mongodb: stepdown_mongod_host
{% endif %}
