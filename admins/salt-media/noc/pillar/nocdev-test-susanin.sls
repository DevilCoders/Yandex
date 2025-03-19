cluster: nocdev-test-susanin

rt.oauth: {{ salt.yav.get('sec-01f8a9yw7njwm4jxhfnfsx8t1h[rt.oauth]') | json }}
tvm.secret: {{ salt.yav.get('sec-01f8a9yw7njwm4jxhfnfsx8t1h[tvm.secret]') | json }}
zk.prefix: /susanin/testing
nginx.server_name: test.susanin.yandex-team.ru

include:
  - units.ssl.nocdev-test-susanin
