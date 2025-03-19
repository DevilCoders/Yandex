{% set hbase_config_path = salt['pillar.get']('data:config:hbase_config_path', '/etc/hbase/conf') %}

{% import 'components/hbase/hbase-site.sls' as c with context %}

{{ hbase_config_path }}/hbase-site.xml:
    hadoop-property.present:
        - config_path: {{ hbase_config_path }}/hbase-site.xml
        - properties:
            {{ c.config_site['hbase'] | json }}

patch-{{ hbase_config_path }}/hbase-env.sh:
    file.append:
        - name: {{ hbase_config_path }}/hbase-env.sh
        - text:
            - ' '
            - '# Forcing IPv4 setting for HBase servers'
            - 'export HBASE_MASTER_OPTS="${HBASE_MASTER_OPTS} -Djava.net.preferIPv6Stack=false -Djava.net.preferIPv4Stack=true "'
            - 'export HBASE_REGIONSERVER_OPTS="${HBASE_REGIONSERVER_OPTS} -Djava.net.preferIPv6Stack=false -Djava.net.preferIPv4Stack=true "'
        - require:
            - dns_record: local-required-dns-records-available
            - dns_record: master-required-dns-records-available

{{ hbase_config_path }}/regionservers:
    file.managed:
        - contents:
        {%- for node in salt['ydputils.get_datanodes']() %}
            - {{ node }}
        {%- endfor %}
