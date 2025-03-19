certificates:
  source: 'certs'
  contents:
    slb.yandex-team.ru.key: {{ salt.yav.get('sec-01fp7z8zcgqpw6sg9s55rj4p1b[7F0019DC969C418D0A26514A5500020019DC96_private_key]') | json }}
    slb.yandex-team.ru.crt: {{ salt.yav.get('sec-01fp7z8zcgqpw6sg9s55rj4p1b[7F0019DC969C418D0A26514A5500020019DC96_certificate]') | json }}

  packages: ['nginx']
  services: 'nginx'
  path: '/etc/nginx/ssl'
