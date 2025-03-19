/etc/yandex/selfdns-client/plugins/default:
  file.managed:
    - source: salt://units/netconfiguration/files/etc/yandex/selfdns-client/plugins/default
    - user: root
    - mode: 755

/etc/yandex/selfdns-client/default.conf:
  file.managed:
    - source: salt://units/netconfiguration/files/etc/yandex/selfdns-client/default.conf
    - user: selfdns
    - group: root
    - mode: 660
    - template: jinja

