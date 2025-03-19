{% set yaenv = grains['yandex-environment'] -%}

pt_scripts_package:
  pkg.installed:
    - name: yandex-media-pt-scripts

pt_scripts_cron:
  yafile.managed:
    - makedirs: True
    - names:
      - /etc/cron.d/pt-dsns:
        - source: salt://{{ slspath }}/files/pt-dsns.cron
      - /etc/cron.d/pt-pause-ptosc:
        - source: salt://{{ slspath }}/files/pt-pause-ptosc.cron
