CREATE TABLE cloud_analytics.cpu_x_memory(
   agg String, 
   date DateTime, 
   node_type String, 
   node_mem_type String,
   zone_id String, 
   cores_used_pct_bucket String, 
   memory_10pct Float32, 
   memory_20pct Float32, 
   memory_30pct Float32, 
   memory_40pct Float32, 
   memory_50pct Float32, 
   memory_60pct Float32, 
   memory_70pct Float32, 
   memory_80pct Float32, 
   memory_90pct Float32,
   memory_100pct Float32
) ENGINE = MergeTree() ORDER BY(agg, date, node_type, zone_id, cores_used_pct_bucket)
