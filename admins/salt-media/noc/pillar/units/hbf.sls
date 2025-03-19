sec: {{salt.yav.get('sec-01ekz4zz769s967y65th814d8m') | json}}

hbf_tvm: {{ salt.yav.get('sec-01fskxfj5z9zd95g0wgeedg9s6') | json }}
nginx_conf_tskv_enabled: False  # use our own tskv config: hbf-tskv.conf

certificates:
  contents  :
    hbf.yandex.net.key: {{ salt.yav.get('sec-01g6j3btjwdfbdfd225ba5pnf7[7F001D7692896DC0375CFBDBC30002001D7692_private_key]') | json }}
    hbf.yandex.net.cert: {{ salt.yav.get('sec-01g6j3btjwdfbdfd225ba5pnf7[7F001D7692896DC0375CFBDBC30002001D7692_certificate]') | json }}
  path      : "/etc/nginx/ssl/"
  packages  : ['nginx']
  services  : 'nginx'
