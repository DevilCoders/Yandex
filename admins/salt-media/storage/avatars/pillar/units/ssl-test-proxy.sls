certificates:
  source    : 'certs'
  files     :
    - 'avatars.pd.yandex.net.key'
    - 'avatars.pd.yandex.net.pem'
    - 'avatars-fast.avt.yandex.net.pem'
    - 'avatars-fast.avt.yandex.net.key'
  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
