certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    mds.yandex.net.key: {{ salt.yav.get('sec-01fedvvbgggp1e8t352yeb57rj[6DFF6E826EDEFEEBB2EBCE98569DABA8_private_key]') | json }}
    mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvvbgggp1e8t352yeb57rj[6DFF6E826EDEFEEBB2EBCE98569DABA8_certificate]') | json }}
    {% else %}
    mds.yandex.net.key: {{ salt.yav.get('sec-01fedvje4a5vtt85yh9c4vs4x7[50E37B723B3004AC6E204D1A6DF1B6B1_private_key]') | json }}
    mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvje4a5vtt85yh9c4vs4x7[50E37B723B3004AC6E204D1A6DF1B6B1_certificate]') | json }}
    {% endif %}



  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
