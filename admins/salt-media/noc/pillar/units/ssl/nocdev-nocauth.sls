certificates:
  source: 'certs'
  contents:
    nocauth-idm.yandex.net.key: {{ salt.yav.get('sec-01g13bs3xef4xkksphb74dfwdw[ 7F001C57C3A5D91C4E33439B050002001C57C3_private_key]') | json }}
    nocauth-idm.yandex.net.crt: {{ salt.yav.get('sec-01g13bs3xef4xkksphb74dfwdw[7F001C57C3A5D91C4E33439B050002001C57C3_certificate]') | json }}

  packages: ['nginx']
  services: 'nginx'
  path: '/etc/nginx/ssl'
