
certificates:
  source: 'certs'
  contents:
    grad.yandex.net.key: {{ salt.yav.get('sec-01ejr3qcfkfymwrq21chm0hjzh[7F00101ED3647D25A3A1FBD134000200101ED3_private_key]') | json }}
    grad.yandex.net.pem: {{ salt.yav.get('sec-01ejr3qcfkfymwrq21chm0hjzh[7F00101ED3647D25A3A1FBD134000200101ED3_certificate]') | json }}

  packages: ['nginx']
  services: 'nginx'
  path: '/etc/nginx/ssl'
