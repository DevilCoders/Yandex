monrun-common-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - require:
            - pkg: yamail-monrun-common

/usr/local/yandex/monitoring/files_io.py:
    file.absent

/usr/local/yandex/monitoring/files_io.pyc:
    file.absent

monrun-common-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - require:
            - pkg: yamail-monrun-common

monrun-common-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0440'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - require:
            - pkg: yamail-monrun-common
