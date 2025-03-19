/etc/monrun/salt_whois/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_whois/whois-monitor.py:
  file.managed:
    - source: salt://{{ slspath }}/files/whois-monitor.py
    - makedirs: True
    - mode: 755

noc-python:
  pkg.installed
