cluster : ape-cloud-12-prestable

include:
  - units.push-client.cloud-12

yasmagentnew:
  RAWSTRINGS:
    - 'PORTO="false"'
  instance-getter:
    -  /usr/local/bin/srw_instance_getter.sh {{ grains['yandex-environment'] }} {{ grains['conductor']['root_datacenter'] }}
    -  echo {{ grains['conductor']['fqdn'] }} a_prj_none a_ctype_testing a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_apecloud

juggler_user: root
juggler_daemon_user: root
juggler_hack_porto: True

walle_enabled: True
