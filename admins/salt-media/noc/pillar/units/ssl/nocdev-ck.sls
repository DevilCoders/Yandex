certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    ck-test.yandex-team.ru.key: {{ salt.yav.get('sec-01fknqpvjxr5tjsnm0qg1jhm66[7F001945A15EC678ECAE0E1FB90002001945A1_private_key]') | json }}
    ck-test.yandex-team.ru.pem: {{ salt.yav.get('sec-01fknqpvjxr5tjsnm0qg1jhm66[7F001945A15EC678ECAE0E1FB90002001945A1_certificate]') | json }}
    {% else %}
    ck.yandex-team.ru.key: {{ salt.yav.get('sec-01efpfr709h7avdjqx68q1p731[7F000F2750EE272E8EA161DF5E0002000F2750_private_key]') | json }}
    ck.yandex-team.ru.pem: {{ salt.yav.get('sec-01efpfr709h7avdjqx68q1p731[7F000F2750EE272E8EA161DF5E0002000F2750_certificate]') | json }}
    {% endif %}



  packages  : ['nginx']
  services  : 'nginx'
  path      : "/etc/yandex-certs/"
