monrun-kibana-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0750'
        - user: root
        - group: monitor
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - require:
            - pkg: monrun-kibana-packages

monrun-kibana-configs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-kibana-packages:
    pkg.installed:
        - pkgs:
            - python3.6
            - python3.6-minimal
            - libpython3.6-stdlib
            - libpython3.6-minimal
            - python3-elasticsearch
        - reload_modules: true

/etc/logrotate.d/kibana-monitoring:
    file.managed:
        - source: salt://{{ slspath }}/logrotate.conf
        - mode: 644
        - require:
            - file: monrun-kibana-configs
