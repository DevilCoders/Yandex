certificates:
  source    : 'certs'

  contents  :
    front.intape.yandex.net.pem: {{ [salt.yav.get('sec-01feeaqdey0m1f7vnk2er2pkec[7F0018077FADE695AE93CFC48400020018077F_private_key]'), salt.yav.get('sec-01feeaqdey0m1f7vnk2er2pkec[7F0018077FADE695AE93CFC48400020018077F_certificate]')] | join('\n') | json }}

  path      : "/etc/nginx/ssl/"
  packages  : ['nginx']
  service   : 'nginx'
  check_pkg : 'config-monrun-cert-check'
