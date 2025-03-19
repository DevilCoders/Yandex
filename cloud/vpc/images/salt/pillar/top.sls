base:
  '*':
    - unified_agent
  'image:vpc-accounting':
    - match: grain
    - vpc_accounting
  'image:mr-prober-*':
    - match: grain
    - mr_prober
  'image:mr-prober-api':
    - match: grain
    - mr_prober
    - mr_prober_api
  'image:mr-prober-creator':
    - match: grain
    - mr_prober
    - mr_prober_creator
  'image:mr-prober-agent':
    - match: grain
    - mr_prober
    - mr_prober_agent
