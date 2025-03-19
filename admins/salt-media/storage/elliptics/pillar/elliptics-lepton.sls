include:
  - units.push-client.lepton

certificates:
  contents  :
    lepton.mdst.yandex.net.key: {{ salt.yav.get('sec-01e37z0s3jbsp77y7ak03nwmem[7F000C3C14E8AC167C731F8DBA0002000C3C14_private_key]') | json }}
    lepton.mdst.yandex.net.pem: {{ salt.yav.get('sec-01e37z0s3jbsp77y7ak03nwmem[7F000C3C14E8AC167C731F8DBA0002000C3C14_certificate]') | json }}
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"

yasmagent:
  instance-getter:
    - echo {{ grains['conductor']['fqdn'] }}:7070@lepton a_itype_mdslepton a_ctype_production a_geo_{{grains['conductor']['root_datacenter']}} a_tier_none a_prj_mds
    - echo {{ grains['conductor']['fqdn'] }}:7070@mdscommon a_prj_none a_ctype_production a_geo_{{grains['conductor']['root_datacenter']}} a_tier_lepton a_itype_mdscommon a_cgroup_elliptics-lepton

lepton-push-client-secret: {{ salt.yav.get('sec-01e3mhk96axhgcc9wbn6q8zw6p[client_secret]') | json }}
iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'
walle_enabled: True
