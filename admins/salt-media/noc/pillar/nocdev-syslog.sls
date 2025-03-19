solomon:
  service: rsyslog
  push_endpoint: /rsyslog
  project: noc
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g12p0s45wsfn6pkxrg4k5q68')['client_secret']|json}}
