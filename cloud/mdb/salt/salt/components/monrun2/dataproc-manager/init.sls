
monrun-dataproc-manager-deps:
    pkg.installed:
        - pkgs:
            - grpc-ping: '1.5625648'

monrun-dataproc-manager-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

/etc/monrun/conf.d/tls.conf:
    file.absent

/usr/local/yandex/monitoring/tls.py:
    file.absent
