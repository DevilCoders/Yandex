certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    ck-test.yandex-team.ru.key: {{ salt.yav.get('sec-01eqz6yktnmvcyztrag6h2aac5[7F001174DF18AA3FDE78C353B80002001174DF_private_key]') | json }}
    ck-test.yandex-team.ru.pem: {{ salt.yav.get('sec-01eqz6yktnmvcyztrag6h2aac5[7F001174DF18AA3FDE78C353B80002001174DF_certificate]') | json }}
    {% else %}
    ck.yandex-team.ru.key: {{ salt.yav.get('sec-01efpfr709h7avdjqx68q1p731[7F000F2750EE272E8EA161DF5E0002000F2750_private_key]') | json }}
    ck.yandex-team.ru.pem: {{ salt.yav.get('sec-01efpfr709h7avdjqx68q1p731[7F000F2750EE272E8EA161DF5E0002000F2750_certificate]') | json }}
    {% endif %}



  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
