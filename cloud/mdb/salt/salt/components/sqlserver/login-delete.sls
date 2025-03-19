{% set username = salt['pillar.get']('target-user') %}

login-absent-{{ username|yaml_encode }}:
  mdb_sqlserver.login_absent:
    - name: {{ username|yaml_encode }}
