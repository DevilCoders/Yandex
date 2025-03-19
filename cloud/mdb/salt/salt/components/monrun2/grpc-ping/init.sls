grpc-ping-pkgs:
    pkg.installed:
        - pkgs:
            - grpc-ping: '1.7834234'
        - require:
            - cmd: repositories-ready

monrun-grpc-ping-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
