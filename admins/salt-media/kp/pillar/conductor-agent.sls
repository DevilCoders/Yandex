{% set group = grains['conductor']['group'] %}
conductor_agent:
  clean_conductor_agent_configs: True
  enable_force_yes: True
  enable_safe_downgrade: False
  overwrite: True
  rules:
    - name: kp
      schedule:
        mon: 9-17
        tue: 9-17
        wed: 9-17
        thu: 9-17
        fri: 9-15
