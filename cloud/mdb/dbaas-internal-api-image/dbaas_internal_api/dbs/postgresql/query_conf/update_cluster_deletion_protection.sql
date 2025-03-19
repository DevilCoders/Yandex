SELECT code.update_cluster_deletion_protection(
    i_cid                 => %(cid)s,
    i_deletion_protection => %(deletion_protection)s,
    i_rev                 => %(rev)s
)
