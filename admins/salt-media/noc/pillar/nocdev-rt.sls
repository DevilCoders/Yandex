include:
  - units.racktables

nginx_conf_tskv_enabled: false
nginx_conf: false

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
certificates:
  contents  :
    wildcard.key: {{ salt.yav.get('sec-01g3tn03qda15wgar86z7tg081[1743EE7A088DF6DD7CDE61BC_private_key]') | json }}
    wildcard.pem: {{ salt.yav.get('sec-01g3tn03qda15wgar86z7tg081[1743EE7A088DF6DD7CDE61BC_certificate]') | json }}
  path      : "/etc/nginx/certs"
  packages  : ['nginx']
  services  : 'nginx'

