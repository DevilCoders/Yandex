{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}

include:
    - components.mdb-metrics

{{ deploy_configs('ch_common') }}

{% if salt.pillar.get('data:dbaas:flavor') %}
{{ deploy_configs('dbaas') }}
{% endif %}

{% if salt.pillar.get('data:clickhouse:config:part_log:enabled', True) %}
{{ deploy_configs('ch_merges') }}
{% endif %}

clickhouse-mdb-metrics-cleanup:
    file.absent:
        - names:
            - /etc/mdb-metrics/conf.d/available/ch_db_size.conf
            - /etc/mdb-metrics/conf.d/enabled/ch_db_size.conf
            - /etc/mdb-metrics/conf.d/available/instance_userfault_broken.conf
            - /etc/mdb-metrics/conf.d/enabled/instance_userfault_broken.conf
        - watch_in:
            - service: mdb-metrics-service
