certificates:
  source    : 'certs'
  contents  :
    walle-cms.mds.yandex.net.key: {{ salt.yav.get('sec-01g6nan0rkhvb61mfe4gtx1y7n[7F001D7D96BEA15F397C35573E0002001D7D96_private_key]') | json }}
    walle-cms.mds.yandex.net.pem: {{ salt.yav.get('sec-01g6nan0rkhvb61mfe4gtx1y7n[7F001D7D96BEA15F397C35573E0002001D7D96_certificate]') | json }}

  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
