include:
   - templates.certificates
   - templates.juggler-search
   - templates.unistat-lua

/etc/yandex/statbox-push-client:
    file.directory:
      - user: root
      - group: root
      - dir_mode: 755


/etc/fastcgi2/templates/resizer.conf:
  file.managed:
    - source: salt://files/resizer-intra-front/etc/fastcgi2/templates/resizer.conf
    - mode: 644
    - user: root
    - group: root

#/etc/yandex-hbf-agent/rules.d/48-localoutblock.v4:
#  file.managed:
#    - source: salt://files/etc/yandex-hbf-agent/rules.d/48-localoutblock.any
#    - mode: 644
#    - user: root
#    - group: root
#    - makedirs: False

#/etc/yandex-hbf-agent/rules.d/48-localoutblock.v6:
#  file.managed:
#    - source: salt://files/etc/yandex-hbf-agent/rules.d/48-localoutblock.any
#    - mode: 644
#    - user: root
#    - group: root
#    - makedirs: False
