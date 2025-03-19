{% set log_dir  = '/var/log/dbaas-cron' %}
{% set log_path = '{dir}/mdbdns_upper.log'.format(dir=log_dir) %}
{% set fqdn     = salt.pillar.get('data:dbaas:fqdn', salt.grains.get('fqdn')) %}

include:
    - components.dbaas-cron.tasks.mdbdns_upper

/etc/dbaas-cron/modules/mdbdns_elasticsearch.py:
    file.managed:
        - source: salt://components/dbaas-cron/conf/mdbdns_elasticsearch.py
        - template: jinja
        - mode: '0640'
        - user: root
        - group: monitor
        - require:
            - pkg: dbaas-cron
            - file: /etc/dbaas-cron/modules/mdbdns_upper.py
        - watch_in:
            - service: dbaas-cron

/etc/dbaas-cron/conf.d/mdbdns_upper.conf:
    file.absent:
        - require_in:
            - file: /etc/dbaas-cron/conf.d/mdbdns_elasticsearch.conf

/etc/dbaas-cron/conf.d/mdbdns_elasticsearch.conf:
    file.managed:
        - template: jinja
        - source: salt://components/dbaas-cron/conf/task_template.conf
        - mode: '0640'
        - user: root
        - group: monitor
        - context:
            module: mdbdns_elasticsearch
            schedule:
                trigger: "interval"
                seconds: 8
            args:
                log_file: {{ log_path }}
                rotate_size: 10485760
                params:
                    fqdn: {{ fqdn }}
                    cid: {{ salt.pillar.get('data:dbaas:cluster_id') }}
                    ca_path: "/opt/yandex/allCAs.pem"
                    send_timeout_ms: 3000
                    cid_key_path: "/etc/dbaas-cron/cid_key.pem"
                    base_url: https://{{ salt.pillar.get('data:mdb_dns:host', 'mdb-dns-test.db.yandex.net') }}
                    ssl_enabled: {{ salt.mdb_clickhouse.ssl_enabled() }}
        - require:
            - pkg: dbaas-cron
            - file: {{ log_dir }}
        - watch_in:
            - service: dbaas-cron
