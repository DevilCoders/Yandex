{% set log_dir = '/var/log/dbaas-cron' %}
{% set log_path = '{dir}/mdbdns_upper.log'.format(dir=log_dir) %}
{% set fqdn = salt['pillar.get']('data:dbaas:fqdn', salt['grains.get']('fqdn')) %}

include:
    - components.dbaas-cron.tasks.mdbdns_upper

/etc/dbaas-cron/modules/mdbdns_mysql.py:
    file.managed:
        - source: salt://components/dbaas-cron/conf/mdbdns_mysql.py
        - mode: '0640'
        - user: root
        - group: monitor
        - require:
            - pkg: dbaas-cron
            - file: /etc/dbaas-cron/modules/mdbdns_upper.py
        - watch_in:
            - service: dbaas-cron

/etc/dbaas-cron/conf.d/mdbdns_mysql.conf:
    file.managed:
        - template: jinja
        - source: salt://components/dbaas-cron/conf/task_template.conf
        - mode: '0640'
        - user: root
        - group: monitor
        - context:
            module: mdbdns_mysql
            schedule:
                trigger: "interval"
                seconds: 8
            args:
                log_file: {{ log_path }}
                rotate_size: 10485760
                params:
                    fqdn: {{ fqdn }}
                    cid: {{ salt['pillar.get']('data:dbaas:cluster_id') }}
                    ca_path: "/opt/yandex/allCAs.pem"
                    send_timeout_ms: 3000
                    cid_key_path: "/etc/dbaas-cron/cid_key.pem"
                    base_url: https://{{ salt['pillar.get']('data:mdb_dns:host', 'mdb-dns-test.db.yandex.net') }}
                    zk_timeout: 3 
{% set zk_hosts = salt['pillar.get']('data:mysql:zk_hosts', ['zk-test01i.mail.yandex.net:2181', 'zk-test01f.mail.yandex.net:2181', 'zk-test01h.mail.yandex.net:2181']) %}
                    zk_hosts: {{ zk_hosts|join(",") }}
        - require:
            - pkg: dbaas-cron
            - file: {{ log_dir }}
        - watch_in:
            - service: dbaas-cron
