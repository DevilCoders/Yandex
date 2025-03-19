certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    chk-test.yandex-team.ru.key: {{ salt.yav.get('sec-01fknqpry006wdxpcte6kv51z2[7F001945A0D3C45A00DCE95CCA0002001945A0_private_key]') | json }}
    chk-test.yandex-team.ru.pem: {{ salt.yav.get('sec-01fknqpry006wdxpcte6kv51z2[7F001945A0D3C45A00DCE95CCA0002001945A0_certificate]') | json }}
    {% else %}
    chk.yandex-team.ru.key: {{ salt.yav.get('sec-01efpfr47htp08jd3qy4fcv9sa[7F000F274FBF72B77DD82FB64D0002000F274F_private_key]') | json }}
    chk.yandex-team.ru.pem: {{ salt.yav.get('sec-01efpfr47htp08jd3qy4fcv9sa[7F000F274FBF72B77DD82FB64D0002000F274F_certificate]') | json }}
    {% endif %}



  packages  : ['nginx']
  services  : 'nginx'
  path      : "/etc/yandex-certs/"
