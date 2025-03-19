include:
  - templates.certificates
  - units.rbtorrent
  - units.hbf-virtual
  - templates.yasmagent

monrun-nginx-rbtorrent-ping:
  monrun.present:
    - name: rbtorrent-ping
    - command: '/usr/bin/http_check.sh ping 80'
    - execution_interval: 60
