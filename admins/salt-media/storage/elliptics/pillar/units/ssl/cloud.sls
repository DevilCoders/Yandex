certificates:
  source    : 'certs'

  contents  :
    allCAs.pem: {{ salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n[allCAs.pem]') | json }}

  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
