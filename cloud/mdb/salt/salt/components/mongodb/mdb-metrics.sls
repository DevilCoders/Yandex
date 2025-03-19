{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
include:
    - components.mdb-metrics

{{ deploy_configs('mongodb_common') }}
{% if mongodb.use_mongos %}
{{ deploy_configs('mongodb_sharding') }}
{% endif %}
{% if salt.pillar.get('data:dbaas:flavor') %}
{{ deploy_configs('dbaas') }}
{% endif %}

extend:
    mdb-metrics-service:
        service.running:
            - require_in:
{% for srv in salt.mdb_mongodb_helpers.services_deployed() if salt.mdb_mongodb_helpers.deploy_service(srv) %}
                - mdb_mongodb: {{srv}}-service
                - service: {{srv}}-service
{% endfor %}

mongodb-mdb-metrics-cleanup:
    file.absent:
        - names:
            - /etc/mdb-metrics/conf.d/available/cpu_load_monitor.conf
            - /etc/mdb-metrics/conf.d/enabled/cpu_load_monitor.conf
            - /etc/mdb-metrics/conf.d/available/instance_userfault_broken.conf
            - /etc/mdb-metrics/conf.d/enabled/instance_userfault_broken.conf
        - watch_in:
            - service: mdb-metrics-service
