include:
  - units.push-client.lepton-test

certificates:
  contents  :
    lepton.mdst.yandex.net.key: {{ salt.yav.get('sec-01fpwna8ka99kqn2wya3zmxdnp[7F001A0714692AF3FAE217E15D0002001A0714_private_key]') | json }}
    lepton.mdst.yandex.net.pem: {{ salt.yav.get('sec-01fpwna8ka99kqn2wya3zmxdnp[7F001A0714692AF3FAE217E15D0002001A0714_certificate]') | json }}
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"

yasmagent:
  instance-getter:
    - echo {{ grains['conductor']['fqdn'] }}:7070@lepton a_itype_mdslepton a_ctype_testing a_geo_{{grains['conductor']['root_datacenter']}} a_tier_none a_prj_mds
    - echo {{ grains['conductor']['fqdn'] }}:7070@mdscommon a_prj_none a_ctype_testing a_geo_{{grains['conductor']['root_datacenter']}} a_tier_lepton a_itype_mdscommon a_cgroup_elliptics-test-lepton
