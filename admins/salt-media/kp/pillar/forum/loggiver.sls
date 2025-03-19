loggiver:
  lookup:
    packages:
      - yandex-3132-fastcgi-loggiver
    service: loggiver
    dpath: /etc/yandex/loggiver/loggiver.pattern
    spath: salt://{{ slspath }}/files/etc/yandex/loggiver/loggiver.pattern
