certificates:
  source    : 'certs'

  contents  :
    avatars.mdst.yandex.net.key: {{ salt.yav.get('sec-01fedvja4sprg3ygpaxxpm40pj[57BA70A62307B8F5873DFDFE09A1CA1E_private_key]') | json }}
    avatars.mdst.yandex.net.pem: {{ salt.yav.get('sec-01fedvja4sprg3ygpaxxpm40pj[57BA70A62307B8F5873DFDFE09A1CA1E_certificate]') | json }}

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  service  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
