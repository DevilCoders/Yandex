include:
    - components.dbaas-cron
    - components.pushclient2
    - components.monrun2.mdb-report

/etc/dbaas-cron/conf.d/offline_billing.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/offline_billing.conf' }}
        - require:
            - pkg: dbaas-cron
            - file: /home/monitor/.postgresql/root.crt
        - watch_in:
            - service: dbaas-cron

/etc/dbaas-cron/modules/offline_billing.py:
    file.managed:
        - source: salt://{{ slspath + '/conf/offline_billing.py' }}
        - require:
            - pkg: dbaas-cron
        - watch_in:
            - service: dbaas-cron

/var/log/dbaas-billing:
    file.directory:
        - user: monitor
        - group: monitor
        - require:
            - pkg: pushclient

/home/monitor/.postgresql:
    file.directory:
        - user: monitor
        - group: monitor
        - require:
            - pkg: pushclient

/home/monitor/.postgresql/root.crt:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - require:
            - file: /home/monitor/.postgresql

/etc/pushclient/offline-billing.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/push-client-offline-billing.conf
        - template: jinja
        - makedirs: True
        - require:
            - file: /var/log/dbaas-billing
            - pkg: pushclient
        - watch_in:
            - service: pushclient

/etc/pushclient/offline-billing.secret:
    file.managed:
{% if salt['pillar.get']('data:mdb_report:offline_billing:use_cloud_logbroker') %}
        - contents: |
              {{ salt['pillar.get']('data:mdb_report:offline_billing:service_account') | json }}
{% elif salt['pillar.get']('data:mdb_report:offline_billing:tvm_client_id') %}
        - contents_pillar: 'data:mdb_report:offline_billing:tvm_secret'
{% else %}
        - contents_pillar: 'data:mdb_report:offline_billing:oauth_token'
{% endif %}
        - user: statbox
        - group: statbox
        - mode: 600
        - require:
            - file: /etc/pushclient/offline-billing.conf
        - watch_in:
            - service: pushclient

/var/lib/push-client-offline-billing:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - require:
            - user: statbox-user
            - pkg: pushclient
        - require_in:
            - file: /etc/pushclient/offline-billing.conf
