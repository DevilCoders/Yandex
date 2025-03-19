{% set do_restart = salt.pillar.get('service-restart') and salt.pillar.get('service-for-restart', None) in ['mongocfg', None] %}
include:
    - components.mongodb.mongocfg-set-feature-compatibility-version
    - components.mongodb.mongocfg-config
    - components.mongodb.mongocfg-service
{% if do_restart %}
    - components.mongodb.mongocfg-restart
{% endif %}
{% if salt.pillar.get('service-stepdown') %}
    - components.mongodb.mongocfg-stepdown-host
{% endif %}

extend:
    mongocfg-service:
        service.running:
            - watch:
                - file: /etc/mongodb/mongocfg.conf
{% if do_restart and salt.pillar.get('service-stepdown') %}
    mongocfg-restart-prereq:
      test.nop:
        - require:
          - mdb_mongodb: stepdown_mongocfg_host
{% endif %}
