yasmagent:
    service.dead:
        - prereq:
            - pkg: yasmagent
    pkg.purged:
        - name: yandex-yasmagent

/lib/systemd/system/yasmagent.service:
    file.absent:
        - onchanges_in:
            - module: systemd-reload
