{% set cluster = pillar.get('cluster') %}

include:
  - units.mastermind
  - units.mastermind-monrun
  - units.mds-logbackup
  - units.la_per_core
  - units.monitoring
  - units.iface-ip-conf
  - units.federation
  - templates.certificates
  - templates.elliptics-tls
  - templates.karl-tls
  - templates.mds-distributed-flock
  - templates.parsers
  - templates.yasmagent
