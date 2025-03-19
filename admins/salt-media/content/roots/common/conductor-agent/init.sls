/etc/conductor-agent/conf.d/99-allow-force-yes.conf:
    file.managed:
        - source: salt://common/conductor-agent/99-allow-force-yes.conf
        - mode: 0644
        - user: root
        - group: root
        - replace: True

/etc/conductor-agent/conf.d/98-debianInstaller.conf:
    file.managed:
        - source: salt://common/conductor-agent/98-debianInstaller.conf
        - mode: 0644
        - user: root
        - group: root

conductor-agent:
    service.running:
        - watch:
            - file: /etc/conductor-agent/conf.d/98-debianInstaller.conf
            - file: /etc/conductor-agent/conf.d/99-allow-force-yes.conf
