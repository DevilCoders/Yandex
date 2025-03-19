certificates:
  source    : 'certs'
  files     :
    - 'strm.yandex.net.pem'
    - 'strm.yandex.net.key'
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/nginx/ssl/"

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
