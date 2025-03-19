remove_yandex_netconfig: False
cluster: nocdev-gitlab-backup

sec: {{salt.yav.get('sec-01eqbc4sm818z38ft4ez30hek0') | json}}
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g5gpgqkpkf2req1v4eete52y')['client_secret']|json}}
