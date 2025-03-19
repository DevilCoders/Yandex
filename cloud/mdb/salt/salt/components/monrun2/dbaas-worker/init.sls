monrun-dbaas-worker-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-dbaas-worker-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-dbaas-worker-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0640'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-dbaas-worker-deps:
    pkg.installed:
        - pkgs:
            - python-psycopg2: '2.8.3-2~pgdg18.04+1+yandex0'
            - python3-psycopg2: '2.8.6-2~pgdg18.04+1'
        - require:
            - cmd: repositories-ready
