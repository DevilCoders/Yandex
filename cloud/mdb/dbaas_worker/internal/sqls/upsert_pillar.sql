SELECT code.upsert_pillar(
    i_cid   => %(cluster_cid)s,
    i_rev   => %(cluster_rev)s,
    i_value => %(value)s,
    i_key   => code.make_pillar_key(
        i_cid      => %(cid)s,
        i_subcid   => %(subcid)s,
        i_shard_id => %(shard_id)s,
        i_fqdn     => %(fqdn)s
    )
)
