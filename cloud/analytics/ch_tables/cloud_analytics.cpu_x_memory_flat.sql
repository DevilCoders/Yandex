CREATE TABLE cloud_analytics.cpu_x_memory_flat(
   agg String, 
   date DateTime, 
   node_type String, 
   node_mem_type String,
   zone_id String, 
   cores_used_pct_bucket String, 
   memory_used_pct_bucket String,
   node_count Float64
) ENGINE = MergeTree() ORDER BY(agg, date, node_type, zone_id, cores_used_pct_bucket, memory_used_pct_bucket)
