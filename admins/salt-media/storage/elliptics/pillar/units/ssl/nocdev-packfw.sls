certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    packfw.tst.net.yandex.net.key: {{ salt.yav.get('sec-01eea789bz1hcrhwmtvksk5kpj[7F000EC99966FFAF12C2FDCF110002000EC999_private_key]') | json }}
    packfw.tst.net.yandex.net.pem: {{ salt.yav.get('sec-01eea789bz1hcrhwmtvksk5kpj[7F000EC99966FFAF12C2FDCF110002000EC999_certificate]') | json }}
    {% else %}
    firewall.yandex-team.ru.key: {{ salt.yav.get('sec-01efp3ed6r39qds80whv1f0asq[7F000F2592A9D1941D003B11160002000F2592_private_key]') | json }}
    firewall.yandex-team.ru.pem: {{ salt.yav.get('sec-01efp3ed6r39qds80whv1f0asq[7F000F2592A9D1941D003B11160002000F2592_certificate]') | json }}
    {% endif %}



  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
