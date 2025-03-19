cores-group:
    group.present:
        - system: True
        - name: cores

var_cores_owner:
    file.directory:
        - name: /var/cores
        - user: root
        - group: cores
        - mode: 775
        - require:
            - group: cores-group

/etc/cron.d/cores-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/cores-cleanup
