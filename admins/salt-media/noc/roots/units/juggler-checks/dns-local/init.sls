/etc/monrun/salt_dns-local/dns_local.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/dns_local.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_dns-local/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

