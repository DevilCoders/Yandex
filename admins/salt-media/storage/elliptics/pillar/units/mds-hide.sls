{%- set env =  grains['yandex-environment'] %}

mds-hide-conf-files:
  - /etc/mds-hide/config.json
  - /etc/ubic/service/mds-hide
  - /etc/cron.d/mds-hide-collect-urls

mds-hide-conf-dirs:
  - /var/log/ubic/mds-hide

mds-hide-additional-pkgs:
  - mds-hide
  - zk-flock
