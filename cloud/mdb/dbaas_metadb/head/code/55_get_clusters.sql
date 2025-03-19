CREATE OR REPLACE FUNCTION code.get_clusters(
    i_folder_id         bigint,
    i_limit             integer,
    i_cid               text                DEFAULT NULL,
    i_cluster_name      text                DEFAULT NULL,
    i_env               dbaas.env_type      DEFAULT NULL,
    i_cluster_type      dbaas.cluster_type  DEFAULT NULL,
    i_page_token_name   text                DEFAULT NULL,
    i_visibility        code.visibility     DEFAULT 'visible'
) RETURNS SETOF code.cluster_with_labels AS $$
SELECT fmt.*
  FROM dbaas.clusters c
  LEFT JOIN dbaas.pillar pl ON (c.cid = pl.cid)
  LEFT JOIN dbaas.maintenance_window_settings mws ON (c.cid = mws.cid)
  LEFT JOIN dbaas.maintenance_tasks mt ON (c.cid = mt.cid AND mt.status='PLANNED'::dbaas.maintenance_task_status)
  LEFT JOIN dbaas.backup_schedule b ON (c.cid = b.cid)
  , LATERAL (
    SELECT array_agg(
              (label_key, label_value)::code.label
              ORDER BY label_key, label_value) la
      FROM dbaas.cluster_labels cl
     WHERE cl.cid = c.cid
  ) x
  , LATERAL (
    SELECT array_agg(sg_ext_id ORDER BY sg_ext_id) sg_ids
      FROM dbaas.sgroups
     WHERE sgroups.cid = c.cid
       AND sg_type = 'user') sg,
      code.as_cluster_with_labels(c, b.schedule, pl.value, la,
                                  mws,
                                  mt,
                                  sg.sg_ids) fmt
WHERE c.folder_id = i_folder_id
  AND (i_cid IS NULL OR c.cid = i_cid)
  AND (i_cluster_name IS NULL OR c.name = i_cluster_name)
  AND (i_env IS NULL OR c.env = i_env)
  AND (i_cluster_type IS NULL OR c.type = i_cluster_type)
  AND (i_page_token_name IS NULL OR c.name > i_page_token_name)
  AND code.match_visibility(c, i_visibility)
ORDER BY c.name ASC
LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
