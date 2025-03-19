{% set yaenv = grains['yandex-environment'] %}

include:
  - .nginx
  - .monrun

/usr/bin/tv_front_http_loggiver.pl:
  file.managed:
    - source: salt://{{ slspath }}/files/tv_front_http_loggiver.pl
    - user: root
    - group: root
    - mode: 755

