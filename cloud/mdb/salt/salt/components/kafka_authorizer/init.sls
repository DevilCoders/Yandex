{% set authorizer_version = salt['pillar.get']('data:kafka:authorizer_version', '0.8893078') %}

kafka-authorizer-pkg:
    pkg.installed:
        - pkgs:
            - kafka-authorizer: {{ authorizer_version }}
        - require:
            - pkg: mdb-kafka-pkgs
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
        - require_in:
            - service: mdb-kafka-service
{% endif %}
