/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: salt://files/loggiver/loggiver.pattern
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

yandex-3132-fastcgi-loggiver:
  pkg.installed

gawk:
  pkg.installed

loggiver:
  service.running:
    - enable: True
    - reload: True
    - watch:
      - file: /etc/yandex/loggiver/loggiver.pattern

/etc/yandex/loggiver/pattern.d/resizer_parse.awk:
  file.managed:
    - source: salt://files/loggiver/pattern.d/resizer_parse.awk
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

