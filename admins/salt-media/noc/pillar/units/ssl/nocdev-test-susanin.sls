certificates:
  source    : 'certs'
  contents  :
    test.susanin.yandex-team.ru.key: {{ salt.yav.get('sec-01fvmrft0sp666jqzpgzc5ctb0[7F001B19B031FA1E39645C9F4C0002001B19B0_private_key]') | json }}
    test.susanin.yandex-team.ru.pem: {{ salt.yav.get('sec-01fvmrft0sp666jqzpgzc5ctb0[7F001B19B031FA1E39645C9F4C0002001B19B0_certificate]') | json }}

  packages  : ['nginx']
  services  : 'nginx'
  path      : "/etc/yandex-certs/"
