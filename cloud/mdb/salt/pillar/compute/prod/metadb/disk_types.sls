# NOTE: it's not a jinja, it should pure yaml
data:
  dbaas_metadb:
    disk_type_ids:
       - disk_type_id: 1
         quota_type: ssd
         disk_type_ext_id: local-ssd
         allocation_unit_size: 1
         io_limit_per_allocation_unit: 1
         io_limit_max: 536870912
       - disk_type_id: 2
         quota_type: ssd
         disk_type_ext_id: network-ssd
         allocation_unit_size: 34359738368
         io_limit_per_allocation_unit: 15728640
         io_limit_max: 471859200
       - disk_type_id: 3
         quota_type: hdd
         disk_type_ext_id: network-hdd
         allocation_unit_size: 274877906944
         io_limit_per_allocation_unit: 31457280
         io_limit_max: 251658240
       - disk_type_id: 4
         quota_type: ssd
         disk_type_ext_id: network-ssd-nonreplicated
         allocation_unit_size: 99857989632
         io_limit_per_allocation_unit: 85983232
         io_limit_max: 1048576000
