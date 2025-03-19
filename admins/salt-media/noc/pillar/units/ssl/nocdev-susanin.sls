certificates:
  source    : 'certs'
  contents  :
    susanin.yandex-team.ru.key: {{ salt.yav.get('sec-01fwp12kxp6dn1hynrmn5a2vte[7F001B5C554CB2B08A253A69590002001B5C55_private_key]') | json }}
    susanin.yandex-team.ru.pem: {{ salt.yav.get('sec-01fwp12kxp6dn1hynrmn5a2vte[7F001B5C554CB2B08A253A69590002001B5C55_certificate]') | json }}

  packages  : ['nginx']
  services  : 'nginx'
  path      : "/etc/yandex-certs/"
