certificates:
  source: 'certs'
  contents:
    netmap.net.yandex.net.key: {{ salt.yav.get('sec-01g56k66tqk8mgbbfttyq642qt[ 7F001D32BFE4781E9775A921FC0002001D32BF_private_key]') | json }}
    netmap.net.yandex.net.crt: {{ salt.yav.get('sec-01g56k66tqk8mgbbfttyq642qt[7F001D32BFE4781E9775A921FC0002001D32BF_certificate]') | json }}

  packages: ['nginx']
  services: 'nginx'
  path: '/etc/nginx/ssl'
