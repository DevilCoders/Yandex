include:
  - units.racktables

nginx_conf_tskv_enabled: false
nginx_conf: false

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
certificates:
  contents  :
    wildcard.key: {{ salt.yav.get('sec-01g546ssz4b9g3ctgn2q8x98qv[7F001D2EDE954AA0BDFF5195C50002001D2EDE_private_key]') | json }}
    wildcard.pem: {{ salt.yav.get('sec-01g546ssz4b9g3ctgn2q8x98qv[7F001D2EDE954AA0BDFF5195C50002001D2EDE_certificate]') | json }}
  path      : "/etc/nginx/certs"
  packages  : ['nginx']
  services  : 'nginx'

