mdb-dataproc-manager-pkgs:
    pkg.installed:
        - pkgs:
            - dataproc-manager: '1.9585155'

mdb-dataproc-manager-user:
  user.present:
    - fullname: Dataproc manager system user
    - name: mdb-dataproc-manager
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True

mdb-dataproc-manager-ensure-capability:
    cmd.run:
        - name: setcap 'cap_net_bind_service=+ep' /opt/yandex/dataproc-manager/bin/dataproc-manager
        - unless: getcap /opt/yandex/dataproc-manager/bin/dataproc-manager | grep 'cap_net_bind_service+ep'

mdb-dataproc-manager-supervised:
    supervisord.running:
        - name: mdb-dataproc-manager
        - update: True
        - require:
            - service: supervisor-service
            - user: mdb-dataproc-manager
            - file: /etc/supervisor/conf.d/mdb-dataproc-manager.conf
            - file: /opt/yandex/dataproc-manager/etc/dataproc-manager.yaml
            - cmd: mdb-dataproc-manager-ensure-capability
        - watch:
            - pkg: mdb-dataproc-manager-pkgs
            - file: /etc/supervisor/conf.d/mdb-dataproc-manager.conf
            - file: /opt/yandex/dataproc-manager/etc/dataproc-manager.yaml

/opt/yandex/dataproc-manager/etc/dataproc-manager.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/dataproc-manager.yaml
        - makedirs: True
        - require:
            - pkg: mdb-dataproc-manager-pkgs

/opt/yandex/dataproc-manager/etc/sa_key_for_logging.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/sa_key_for_logging.json
        - makedirs: True

/etc/supervisor/conf.d/mdb-dataproc-manager.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require:
            - pkg: mdb-dataproc-manager-pkgs

/etc/td-agent-bit/parsers.conf:
    file.managed:
        - makedirs: true
        - show_changes: false
        - source: salt://{{ slspath }}/conf/parsers.conf
        - require:
            - pkg: fluentbit-packages

/etc/td-agent-bit/td-agent-bit.conf:
    file.managed:
        - template: jinja
        - makedirs: true
        - show_changes: false
        - source: salt://{{ slspath }}/conf/td-agent-bit.conf
        - require:
            - pkg: fluentbit-packages

service-fluentbit:
    service.running:
        - enable: true
        - name: td-agent-bit
        - require:
            - file: /etc/td-agent-bit/td-agent-bit.conf
            - file: /opt/td-agent-bit/bin/yc-logging.so
            - pkg: fluentbit-packages
        - watch:
            - file: /etc/td-agent-bit/td-agent-bit.conf
            - file: /opt/td-agent-bit/bin/yc-logging.so

/usr/local/share/ca-certificates/yandex-cloud-ca.crt:
  file.managed:
    - source:
      - https://storage.yandexcloud.net/cloud-certs/CA.pem
    - mode: 0644
    - skip_verify: True
  cmd.run:
    - name: 'update-ca-certificates --fresh'
    - onchanges:
      - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt

{% set include_mdb_metrics = salt['pillar.get']('data:mdb_metrics:enabled', True) %}
{% set include_monrun = salt['pillar.get']('data:monrun2', True) %}

{% if include_mdb_metrics or include_monrun %}
include:
    - components.fluentbit
{% if include_mdb_metrics %}
    - .mdb-metrics
{% endif %}
{% if include_monrun %}
    - components.monrun2.dataproc-manager
{% endif %}
{% endif %}
