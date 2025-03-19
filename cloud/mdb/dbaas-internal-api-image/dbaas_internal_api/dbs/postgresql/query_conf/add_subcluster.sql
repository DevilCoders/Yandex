SELECT subcid,
       cid,
       name,
       roles
  FROM code.add_subcluster(
      i_cid    => %(cid)s,
      i_subcid => %(subcid)s,
      i_name   => %(name)s,
      i_roles  => %(roles)s::dbaas.role_type[],
      i_rev    => %(rev)s
)
