SELECT *
  FROM code.update_disk_placement_group(
      i_cid                     => %(cid)s,
      i_disk_placement_group_id => %(disk_placement_group_id)s,
      i_local_id                => %(local_id)s,
      i_status                  => %(status)s,
      i_rev                     => %(rev)s
  )
