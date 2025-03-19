/usr/local/yandex/monitoring/http_tls.py:
    file.managed:
        - mode: '0755'
        - source: salt://{{ slspath }}/scripts/http_tls.py
        - watch_in:
            - cmd: monrun-jobs-update

monrun-http-tls-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - defaults:
            port: 443
            crt_path: ''
        - require:
            - file: /usr/local/yandex/monitoring/http_tls.py
        - watch_in:
            - cmd: monrun-jobs-update
