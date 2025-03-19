sec: {{salt.yav.get('sec-01dz3q76qc2342rgya05m8ma73') | json}}


ssl_key: {{ salt.yav.get('sec-01fek6y6d7agt6z2mp5bz367gp[7F001811371E187C71ADA39E45000200181137_private_key]') | json }}
ssl_cert: {{ salt.yav.get('sec-01fek6y6d7agt6z2mp5bz367gp[7F001811371E187C71ADA39E45000200181137_certificate]') | json }}


include:
  - units.ssl.nocdev-solomon-proxy

nginx_conf_solomon_enabled: false
nginx_conf_tskv_enabled: false
