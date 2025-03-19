{% set cluster = pillar.get('cluster') %}

include:
  - templates.yasmagent
  - templates.push-client
  - units.logshatter
  - units.iface-ip-conf
