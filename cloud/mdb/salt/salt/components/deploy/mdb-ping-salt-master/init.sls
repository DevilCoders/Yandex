mdb-ping-salt-master-service:
    service.running:
        - name: mdb-ping-salt-master
        - enable: True
        - watch:
            - pkg: common-packages

/etc/supervisor/conf.d/mdb-ping-salt-master.conf:
    file.absent

/etc/init/mdb-ping-salt-master.conf.yandex:
    file.absent
