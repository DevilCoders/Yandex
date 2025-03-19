SELECT cid, folder_id, name,
       type, network_id, env,
       rev
  FROM code.create_cluster(
      i_cid                 => %(cid)s,
      i_name                => %(name)s,
      i_type                => %(type)s,
      i_env                 => %(env)s,
      i_public_key          => %(public_key)s,
      i_network_id          => %(network_id)s,
      i_folder_id           => %(folder_id)s,
      i_description         => %(description)s,
      i_x_request_id        => %(x_request_id)s,
      i_host_group_ids      => %(host_group_ids)s,
      i_deletion_protection => %(deletion_protection)s,
      i_monitoring_cloud_id => %(monitoring_cloud_id)s
)
