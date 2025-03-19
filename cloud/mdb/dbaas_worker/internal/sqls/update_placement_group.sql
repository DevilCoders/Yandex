SELECT *
  FROM code.update_placement_group(
      i_cid                     => %(cid)s,
      i_placement_group_id      => %(placement_group_id)s,
      i_status                  => %(status)s,
      i_fqdn                    => %(fqdn)s,
      i_rev                     => %(rev)s
  )
