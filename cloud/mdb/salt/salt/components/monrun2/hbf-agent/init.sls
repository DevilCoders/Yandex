monrun-hbf-agent-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-hbf-agent-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

/etc/yandex-hbf-agent/drops-monitoring-config.yaml:
    file.managed:
        - source: salt://{{ slspath }}/drops-monitoring-config.yaml
        - mode: 0644
        - makedirs: True
