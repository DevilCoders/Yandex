include:
  - units.federation

yasmagentnew:
    instance-getter:
      - echo {{ grains['conductor']['fqdn'] }}:10010@yarl a_prj_none a_ctype_production a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_yarl
