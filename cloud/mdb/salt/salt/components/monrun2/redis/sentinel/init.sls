monrun-sentinel-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-sentinel-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
