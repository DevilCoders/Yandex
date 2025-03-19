{% set ssl_path = '/etc/kafka/ssl' %}

{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}

include:
    - components.mdb-metrics

{% if not salt['pillar.get']('data:running_on_template_machine', False) %}

create-monitor-truststore:
    cmd.run:
        - name: keytool -keystore monitor.truststore.jks -noprompt -alias CARoot -import -file cert-ca.pem -storepass ${PASS}
        - cwd: {{ ssl_path }}
        - env:
            PASS: {{ salt['mdb_kafka.monitor_password']() }}
        - creates:
            - {{ ssl_path }}/monitor.truststore.jks
        - require:
            - file: {{ ssl_path }}/cert-ca.pem

mdb-kafka-remove-jmx-collector:
  file.absent:
    - name: /etc/mdb-metrics/conf.d/enabled/kafka_jmx.conf

{% endif %}

{{ deploy_configs('kafka') }}

{% if salt['pillar.get']('data:mdb_metrics:enable_userfault_broken_collector', True) %}
{{ deploy_configs('instance_userfault_broken') }}
{% endif %}
