/usr/local/bin/alet-yav-getter.sh:
  file.managed:
    - source: salt://{{ slspath }}/alet-yav-getter.sh
    - user: root
    - group: root
    - mode: 755


