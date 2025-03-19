certificates:
  source: 'certs'
  contents:
    pahom.yandex-team.ru.key: {{ salt.yav.get('sec-01esrw8293my7r775tx9h56hev[pahom.yandex-team.ru_ssl_key]') | json }}
    pahom.yandex-team.ru.pem: {{ salt.yav.get('sec-01esrw8293my7r775tx9h56hev[pahom.yandex-team.ru_ssl_pem]') | json }}

  packages: ['nginx']
  services: 'nginx'
  path: '/etc/nginx/ssl'
