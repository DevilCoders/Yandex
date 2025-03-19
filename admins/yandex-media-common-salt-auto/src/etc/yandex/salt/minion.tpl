file_client: local
fileserver_backend:
  - roots

file_roots:
  base:
    - /srv/salt-masterless__REPO__/__PROJECT__/roots
    - /srv/salt-masterless__REPO__/common/roots

pillar_roots:
  base:
    - /srv/salt-masterless__REPO__/__PROJECT__/pillar
    - /srv/salt-masterless__REPO__/common/pillar

ipv6: True
log_level: info
yav.config:
  agent: True
