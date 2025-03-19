certificates:
  source    : 'certs'

  contents  :
    avatars.mds.yandex.net.key: {{ salt.yav.get('sec-01ekcqekpwq18v63jkrf484k8y[43797505EB7EAA66921758E5E2FE9B64_private_key]') | json }}
    avatars.mds.yandex.net.pem: {{ salt.yav.get('sec-01ekcqekpwq18v63jkrf484k8y[43797505EB7EAA66921758E5E2FE9B64_certificate]') | json }}

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
