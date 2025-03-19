{% set yaenv = grains['yandex-environment'] %}

conductor_agent:
  clean_conductor_agent_configs: True
  enable_force_yes: True
  enable_safe_downgrade: False
  overwrite: True
