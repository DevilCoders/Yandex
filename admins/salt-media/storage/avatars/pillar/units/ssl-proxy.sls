certificates:
  source    : 'certs'
  files     :
    - 'cert.avatars-fast.yandex.net.pem'
    - 'cert.avatars.yandex.net.pem'
    - 'cert.cards2.yandex.net.pem'
    - 'key.avatars-fast.yandex.net.pem'
    - 'key.avatars.yandex.net.pem'
    - 'key.cards2.yandex.net.pem'

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
