certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    chk-test.yandex-team.ru.key: {{ salt.yav.get('sec-01eqz6yktnmvcyztrag6h2aac5[7F001174DF18AA3FDE78C353B80002001174DF_private_key]') | json }}
    chk-test.yandex-team.ru.pem: {{ salt.yav.get('sec-01eqz6yktnmvcyztrag6h2aac5[7F001174DF18AA3FDE78C353B80002001174DF_certificate]') | json }}
    {% else %}
    chk.yandex-team.ru.key: {{ salt.yav.get('sec-01efpfr47htp08jd3qy4fcv9sa[7F000F274FBF72B77DD82FB64D0002000F274F_private_key]') | json }}
    chk.yandex-team.ru.pem: {{ salt.yav.get('sec-01efpfr47htp08jd3qy4fcv9sa[7F000F274FBF72B77DD82FB64D0002000F274F_certificate]') | json }}
    {% endif %}



  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
