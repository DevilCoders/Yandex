/etc/yandex/logshatter:
  file.directory:
  - dir_mode: 755

/etc/yandex/logshatter/logshatter-env.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/logshatter/logshatter-env.sh

/etc/yandex/logshatter/logshatter-production.properties:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/logshatter/logshatter-production.properties
    - template: jinja

/etc/yandex/logshatter/conf.d:
  file.recurse:
  - dir_mode: 755
  - file_mode: '0644'
  - source: salt://{{ slspath }}/files/etc/yandex/logshatter/conf.d
  - watch_in:
    - service: logshatter
