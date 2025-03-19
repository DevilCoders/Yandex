# NOTE: it's not a jinja, it should pure yaml
data:
  dbaas_metadb:
    disk_type_ids:
      - disk_type_id: 1
        quota_type: ssd
        disk_type_ext_id: network-ssd
        allocation_unit_size: 34359738368 # 32GiB
        io_limit_per_allocation_unit: 15728640 # 15MiB
        io_limit_max: 471859200 # 450MiB
      - disk_type_id: 2
        quota_type: hdd
        disk_type_ext_id: network-hdd
        allocation_unit_size: 274877906944 # 256GiB
        io_limit_per_allocation_unit: 31457280 # 30MiB
        io_limit_max: 251658240 # 240MiB
