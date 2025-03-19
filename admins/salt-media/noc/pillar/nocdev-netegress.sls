nginx_conf_tskv_enabled: false
nginx_conf: false

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
certificates:
  contents  :
    egress.yandex.net.key: {{ salt.yav.get('sec-01fwr09yhvrgnpce23jwxt15g6[398016B08E80A168A23138EF7AE082C7_private_key]') | json }}
    egress.yandex.net.cert: {{ salt.yav.get('sec-01fwr09yhvrgnpce23jwxt15g6[398016B08E80A168A23138EF7AE082C7_certificate]') | json }}
  path      : "/etc/nginx/ssl/"
  packages  : ['nginx']
  services  : 'nginx'

salt_cert_expires_path: '/etc/nginx/ssl/*.cert'
