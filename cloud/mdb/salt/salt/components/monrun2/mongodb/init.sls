monrun-mongodb-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - require:
            - pkg: monrun-mongodb-packages

monrun-mongodb-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - require:
            - file: monrun-mongodb-scripts
        - watch_in:
            - cmd: monrun-jobs-update

monrun-mongodb-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0440'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

/etc/sudoers.d/02_monitor_mongodb_shards_primaries:
    file.absent:
        - require_in:
          - monrun-mongodb-confs

/etc/monrun/conf.d/walg_backup_age.conf:
    file.absent:
        - require_in:
          - monrun-mongodb-confs

monrun-mongodb-packages:
    pkg.installed:
        - pkgs:
            - python3.6
            - python3.6-minimal
            - libpython3.6-stdlib
            - libpython3.6-minimal
            - python3-requests
            - python3-yaml
            - python3-openssl
            - python3-pip
            - python3-psutil
            - python3-iso8601
        - reload_modules: true
