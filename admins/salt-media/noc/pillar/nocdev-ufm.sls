cluster: nocdev-ufm

ufm_srv_types:
  baremetal:
    opensm_conf: /opt/ufm/files/conf/opensm/opensm.conf
    root_guid_conf: /opt/ufm/files/conf/opensm/root_guid.conf
  docker:
    opensm_conf: /opt/ufm/files/conf/opensm/opensm.conf
    root_guid_conf: /opt/ufm/files/conf/opensm/root_guid.conf
  opensm:
    opensm_conf: /etc/opensm/opensm.conf
    root_guid_conf: /etc/opensm/root_guid.conf

ufm_hosts:
  sas-gpuib1-osm-master.yndx.net: opensm
  sas-gpuib1-osm-slave.yndx.net: opensm
  sas2-10-ufm-master.yndx.net: docker
  sas2-10-ufm-slave.yndx.net: opensm
  vla2-10-ufm-master.yndx.net: docker
  vla2-10-ufm-slave.yndx.net: opensm
  vla-gpuib4-osm-master.yndx.net: opensm
  vla2-12-ufm-slave.yndx.net: opensm

