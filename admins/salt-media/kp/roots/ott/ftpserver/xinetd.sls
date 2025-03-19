xinetd:
    pkg.installed:
        - name: xinetd
    service.running:
        - enable: True
        - name: xinetd
        - watch:
            - file: /etc/xinetd.d/vsftpd

/usr/bin/vsftpd-watchdog.sh:
    file.managed:
        - user: root
        - source: salt://{{ slspath }}/files/usr/bin/vsftpd-watchdog.sh
        - group: root
        - mode: 755

