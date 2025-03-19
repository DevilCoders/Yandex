/etc/sysctl.d/ZZ-All-common.conf:
    file.managed:
        - source: salt://units/sysctl_all/ZZ-All-common.conf
        - user: root
        - group: root
        - mode: 644

/etc/yandex-hbf-agent/rules.d/31-4sack.v4:
    file.managed:
        - source: salt://units/sysctl_all/31-4sack.any
        - user: root
        - group: root
        - mode: 644
        - makedirs: True

/etc/yandex-hbf-agent/rules.d/31-4sack.v6:
    file.managed:
        - source: salt://units/sysctl_all/31-4sack.any
        - user: root
        - group: root
        - mode: 644
        - makedirs: True
