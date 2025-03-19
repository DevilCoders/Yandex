monrun-jenkins-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-jenkins-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-jenkins-deps:
    pkg.installed:
        - pkgs:
            - python3-requests
        - require:
            - cmd: repositories-ready

/etc/monrun/conf.d/terraform_plan_diff.conf:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update

/usr/local/yandex/monitoring/terraform_plan_diff.py:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update
