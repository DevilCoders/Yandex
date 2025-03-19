SELECT code.update_pillar(
      i_value => %(value)s,
      i_cid   => %(cid)s,
      i_rev   => %(rev)s,
      i_key   => code.make_pillar_key(
        i_cid      => %(pillar_cid)s,
        i_subcid   => %(pillar_subcid)s,
        i_shard_id => %(pillar_shard_id)s,
        i_fqdn     => %(pillar_fqdn)s
      )
  )