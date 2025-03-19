SELECT *
  FROM code.update_disk(
      i_cid          => %(cid)s,
      i_local_id     => %(local_id)s,
      i_fqdn         => %(fqdn)s,
      i_mount_point  => %(mount_point)s,
      i_rev          => %(rev)s,
      i_status       => %(status)s,
      i_disk_id      => %(disk_id)s,
      i_host_disk_id => %(host_disk_id)s
  )
