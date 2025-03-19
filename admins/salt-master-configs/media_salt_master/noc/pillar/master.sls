arc2salt:
  api-token: {{ salt.yav.get('sec-01epynddbcmerbgpr0rmgm2y5n[arc-api-token]') | json }}
  arc-token: {{ salt.yav.get('sec-01epynddbcmerbgpr0rmgm2y5n[arc-token]') | json }}
  arc-group: nocdev-salt # /trunk/arcadia/groups/
  arc-path: admins/salt-media/
  projects:
    - noc
    - common
  add-branches:
    - trunk

master:
  user: robot-nocdev-salt
  private_key: {{ salt.yav.get('sec-01epynddbcmerbgpr0rmgm2y5n[salt_master_private]') | json }}
