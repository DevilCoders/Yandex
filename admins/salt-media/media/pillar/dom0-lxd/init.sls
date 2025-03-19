packages:
  cluster:
    - yandex-media-common-lxd-bootstrap
    - yandex-media-common-hbf-agent
    - yandex-hbf-agent-monitoring

lxd:
  init:
    backend: lvm
    backend_opts: "lvm.use_thinpool=false"
    pool: lxd
  secrets:
    crt: {{ salt.yav.get('sec-01d1vrdba5bkg6vvs2fwm9eaw0[crt]') | json }}
    key: {{ salt.yav.get('sec-01d1vrdba5bkg6vvs2fwm9eaw0[key]') | json }}
    pass: {{ salt.yav.get('sec-01d1vrdba5bkg6vvs2fwm9eaw0[pass]') | json }}

nginx:
  lookup:
    log_params:
      name: 'media-dom0-lxd-access-log'
      access_log_name: access.log
