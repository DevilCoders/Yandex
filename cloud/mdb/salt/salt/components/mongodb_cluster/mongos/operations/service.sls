{% set do_restart = salt.pillar.get('service-restart') and salt.pillar.get('service-for-restart', None) in ['mongos', None] %}
include:
    - components.mongodb.mongos-config
    - components.mongodb.mongos-service
{% if do_restart %}
    - components.mongodb.mongos-restart
{% endif %}

extend:
    mongos-service:
        service.running:
            - watch:
                - file: /etc/mongodb/mongos.conf
