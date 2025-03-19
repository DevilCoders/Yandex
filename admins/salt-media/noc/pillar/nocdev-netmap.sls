racktables_creds: {{ salt.yav.get('sec-01etcyhcw2ee0zb50x3henn2s9') | json }}
racktables_password_old: {{ salt.yav.get('ver-01etcyhcwmd9zdyf8pmw835zfv[password]') | json }}
tmv_secret: {{ salt.yav.get('sec-01fte2styj7c3kd3z80c2brsj1[client_secret]') | json }}
invapi_oath_token: {{ salt.yav.get('sec-01g6r0qnm6aqxskcg9anzjq957[invapi_oauth]') | json }}
robot_netmap: {{ salt.yav.get('sec-01fzwm6dk5z9ak2ptjg49t9dwg') | json }}
sec_rt_yandex: {{ salt.yav.get('sec-01ex7wbyyw047qm8gmb819cpea') | json }}

include:
  - units.ssl.nocdev-netmap
