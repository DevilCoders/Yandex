SELECT f.folder_id,
       f.folder_ext_id,
       f.cloud_id
  FROM dbaas.clouds c
  JOIN dbaas.folders f
 USING (cloud_id)
 WHERE cloud_ext_id = %(cloud_ext_id)s