monrun-zookeeper-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-zookeeper-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-zookeeper-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0640'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

/usr/local/yandex/monitoring/zk_alive.sh:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update

/usr/local/yandex/monitoring/zk_mntr.sh:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update

zk-monitoring-deps:
    pkg.installed:
        - pkgs:
            - python3-kazoo

