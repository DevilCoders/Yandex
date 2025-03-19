SELECT *
  FROM code.update_host(
      i_fqdn      => %(fqdn)s,
      i_subnet_id => %(subnet_id)s,
      i_cid       => %(cid)s,
      i_rev       => %(rev)s
  )
