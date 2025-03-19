{% if 'resizer-intra-front' in grains['c'] %}

certificates:
  source    : 'certs-yt'
  contents  :
    key.resize.yandex.net.pem: {{ salt.yav.get('sec-01fedvj8dqxs1qjkew2zvajqht[633DEC511D74B1D33806891D147D149B_private_key]') | json }}
    cert.resize.yandex.net.pem: {{ salt.yav.get('sec-01fedvj8dqxs1qjkew2zvajqht[633DEC511D74B1D33806891D147D149B_certificate]') | json }}

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'

{% elif 'resizer-front' in grains['c'] %}

certificates:
  source    : 'certs'
  contents  :
    key.resize.yandex.net.pem: {{ salt.yav.get('sec-01fedvj6awc65tre1qhkkkq609[100EE71C08E6D4AB52A78F83678F08FB_private_key]') | json }}
    cert.resize.yandex.net.pem: {{ salt.yav.get('sec-01fedvj6awc65tre1qhkkkq609[100EE71C08E6D4AB52A78F83678F08FB_certificate]') | json }}

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'

{% endif %}
