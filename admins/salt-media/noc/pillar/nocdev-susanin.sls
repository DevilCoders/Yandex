cluster: nocdev-susanin

rt.oauth: {{ salt.yav.get('sec-01f8a9yw7njwm4jxhfnfsx8t1h[rt.oauth]') | json }}
tvm.secret: {{ salt.yav.get('sec-01f8a9yw7njwm4jxhfnfsx8t1h[tvm.secret]') | json }}
zk.prefix: /susanin/production
nginx.server_name: susanin.yandex-team.ru

include:
  - units.ssl.nocdev-susanin
