certificates:
  source    : 'certs-resizer'
  contents  :
    key.resize.yandex.net.pem: {{ salt.yav.get('sec-01feb80w550eyneatxvtp2kv73[7F0017FFE330D99E44D383B1C600020017FFE3_private_key]') | json }}
    cert.resize.yandex.net.pem: {{ salt.yav.get('sec-01feb80w550eyneatxvtp2kv73[7F0017FFE330D99E44D383B1C600020017FFE3_certificate]') | json }}

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'


include:
  - units.new-juggler-client
  - units.resizer-secrets
