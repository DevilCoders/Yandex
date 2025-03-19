#Telegraf status
/etc/monrun/salt_telegraf/telegraf_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/telegraf_status.sh
    - makedirs: True
    - mode: 755
#  pkg.installed:
#      - pkgs:
#        - telegraf-noc-conf
/etc/monrun/salt_telegraf/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

