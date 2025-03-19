{% set log_dir = '/var/log/dbaas-cron' %}
{% set log_path = '{dir}/cron.log'.format(dir=log_dir) %}
{% set fqdn = salt['pillar.get']('data:dbaas:fqdn', salt['grains.get']('fqdn')) %}

include:
    - .service

{{ log_dir }}:
    file.directory:
        - user: monitor
        - group: monitor

/etc/logrotate.d/dbaas-cron:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - template: jinja
        - mode: 644
        - require:
            - file: {{ log_dir }}
            - pkg: dbaas-cron

extend:
    dbaas-cron:
        pkg.installed:
            - pkgs:
                - dbaas-cron: 17-456380a
                - libpython3.6
                - libpython3.6-minimal
                - libpython3.6-stdlib
                - python3.6
                - python3.6-minimal
                - python3.6-venv
                - libmpdec2
            - prereq_in:
                - cmd: repositories-ready
            - watch_in:
                - service: dbaas-cron

dbaas-cron-service-enabled:
    service.enabled:
        - name: dbaas-cron
        - require:
            - pkg: dbaas-cron
        - require_in:
            - service: dbaas-cron

/etc/dbaas-cron/dbaas-cron.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/dbaas-cron.conf
        - mode: '0640'
        - user: root
        - group: monitor
        - require:
            - pkg: dbaas-cron
        - watch_in:
            - service: dbaas-cron

/etc/cron.d/wd-dbaas-cron:
    file.managed:
        - source: salt://{{ slspath }}/conf/wd-dbaas-cron.cron
        - require:
            - file: /etc/dbaas-cron/dbaas-cron.conf
            - pkg: dbaas-cron

/etc/cron.yandex/wd-dbaas-cron.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/wd-dbaas-cron.sh
        - template: jinja
        - mode: 755
        - require:
            - file: /etc/dbaas-cron/dbaas-cron.conf
            - pkg: dbaas-cron

/lib/systemd/system/dbaas-cron.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbaas-cron.service
        - require:
            - pkg: dbaas-cron
        - require_in:
            - service: dbaas-cron
        - onchanges_in:
            - module: systemd-reload
