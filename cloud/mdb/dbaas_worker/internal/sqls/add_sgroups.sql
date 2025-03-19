INSERT INTO dbaas.sgroups SELECT %(cid)s, sg_ids, %(sg_type)s, %(sg_hash)s, %(sg_allow_all)s FROM (SELECT unnest(string_to_array(%(sg_ext_ids)s, ','))) as t (sg_ids)
ON CONFLICT DO NOTHING
