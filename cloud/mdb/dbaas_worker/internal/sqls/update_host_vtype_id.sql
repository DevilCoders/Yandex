SELECT *
  FROM code.update_host(
      i_fqdn     => %(fqdn)s,
      i_vtype_id => %(vtype_id)s,
      i_cid      => %(cid)s,
      i_rev      => %(rev)s
  )
