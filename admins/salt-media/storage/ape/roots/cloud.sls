/etc/yandex/statbox-push-client:
  file.recurse:
    - source: salt://cloud/yandex/statbox-push-client/

/etc/apt/sources.list.d/cocaine.list:
  file.managed:
    - source: salt://cocaine.list

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://cloud/cocaine.conf
    - template: jinja

/etc/cocaine/cadaver-handler.ini:
  file.managed:
    - source: salt://cloud/cadaver-handler.ini

include:
  - corehandler

