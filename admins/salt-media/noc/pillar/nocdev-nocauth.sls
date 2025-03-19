sec: {{ salt.yav.get('sec-01fy9crpkr3brxf5cadtph8dxc') | json }}
monitor: {{ salt.yav.get('sec-01g1npamj6ctphz6fnt5phz9r2') | json }}

include:
  - units.ssl.nocdev-nocauth
