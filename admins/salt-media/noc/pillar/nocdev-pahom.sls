cluster: nocdev-pahom

include:
  - units.ssl.nocdev-pahom

sec: {{salt.yav.get('sec-01esrw8293my7r775tx9h56hev') | json}}
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g6qhen37ps5ak3q2axtcfv4m')['client_secret']|json}}

nginx_conf_solomon_enabled: false
nginx_conf_tskv_enabled: false

