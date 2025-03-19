/usr/lib/yandex-graphite-checks/enabled/sessions.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/sessions.sh
    - mode: 755
