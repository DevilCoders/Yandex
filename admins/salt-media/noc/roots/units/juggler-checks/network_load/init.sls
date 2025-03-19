/etc/monrun/salt_network_load/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - config-monitoring-network-load

/etc/monrun/salt_network_load/network_load.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/network_load.sh
    - makedirs: True
    - mode: 755


{% if grains['osrelease'] == "20.04" %}
patch python3 dstat:
  file.patch:
    - name: /usr/bin/dstat
    - source: salt://{{ slspath }}/files/dstat.patch
    - contents: |
{% endif %}
