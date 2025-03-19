iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
certificates:
  contents  :
    api.puncher.yandex-team.ru.key: {{ salt.yav.get('sec-01fek77bq5kp634dygr0bqa8wh[7F0018113A35ECE7DA65DBC00C00020018113A_private_key]') | json }}
    api.puncher.yandex-team.ru.pem: {{ salt.yav.get('sec-01fek77bq5kp634dygr0bqa8wh[7F0018113A35ECE7DA65DBC00C00020018113A_certificate]') | json }}
  path      : "/etc/nginx/ssl/"
  packages  : ['nginx'] 
  services  : 'nginx'

salt_cert_expires_path: '/etc/puncher/private/cauth.cer'
salt_cert_expires_service: 'puncher_certcheck'
