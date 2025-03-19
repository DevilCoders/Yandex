#clickhouse_zookeeper_hosts: {{ salt['conductor']['groups2hosts']('elliptics-zk') | list }}
clickhouse_zookeeper_hosts:
  - clickhouse01iva.mds.yandex.net
  - clickhouse04myt.mds.yandex.net
  - clickhouse05sas.mds.yandex.net

yasmagent:
    instance-getter:
      {% for itype in ['ellipticsclickhouse'] %}
      - echo {{ grains['conductor']['fqdn'] }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}
      - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_{{grains['conductor']['group']}}

salt_state_command_args: "-w 4 -c 10 -a test=False"
