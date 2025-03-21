CREATE TABLE %(table_name)s
(
    vm_id String,
    vm_mdb String,
        vm_start DateTime,
        vm_finish DateTime,
        slice_time DateTime,
        slice_is_last UInt64,
        slice_time_str String,
        vm_is_in_slice String,
        vm_age UInt64,
        vm_age_months Float64,
        vm_age_weeks Float64,
        vm_age_days Float64,
        slice_time_day Date,
        slice_time_week Date,
        slice_time_month Date,
        slice_time_quarter Date,
        ba_id String,
        cloud_id String,
        cloud_name String,
        folder_id String,
        folder_name String,
        node_id String,
        node_az String,
        vm_cores Float64,
        vm_gpus UInt64,
        vm_core_fraction Float64,
        vm_cores_real Float64,
        vm_cores_effective_shared Float64,
        vm_cores_effective_shared_and_free Float64,
        vm_memory Float64,
        vm_memory_real Float64,
        vm_config String,
        vm_memory_to_cores_ratio Float64,
        node_platform String,
        vm_preemptible UInt64,
        vm_public_fips UInt64,
        vm_product_ids String,
        vm_product_id String,
        vm_product_name String,
        vm_labels_str String,
        vm_managed_kubernetes_cluster_id String,
        vm_managed_kubernetes_node_group_id String,
        vm_managed_kubernetes_cluster_size String,
        vm_managed_kubernetes_cluster_type String,
        vm_origin String,
        ba_is_fraud UInt64, 
        ba_name String, 
        ba_person_type String, 
        ba_type String, 
        ba_channel String, 
        ba_segment String, 
        ba_board_segment String, 
        ba_user_email String, 
        ba_client_name String, 
        ba_state String, 
        vm_is_service String, 
        ba_block_reason String, 
        ba_sales_name String, 
        ba_architect String, 
        ba_paid_status String, 
        ba_m_cohort String,
        ba_w_cohort String,
        node_cores_free Float64,
        node_memory_free Float64,
        node_on_demand_shared_cores_used Float64,
        node_cores_disabled Float64,
        node_shared_cores Float64,
        node_preemptible_shared_cores_used Float64,
        node_cores_total Float64,
        node_preemptible_exclusive_cores_used Float64,
        node_shared_cores_available Float64,
        node_memory_disabled Float64,
        node_on_demand_exclusive_cores_used Float64,
        node_memory_total Float64,
        vm_price_1h_cpu Float64,
        vm_price_1h_ram Float64,
        vm_price_1h_gpu Float64,
        vm_price_lifetime Float64,
        vm_price_1h Float64,
        vm_price_4h Float64,
        vm_cost_1h_cpu Float64,
        vm_cost_1h_ram Float64,
        vm_cost_1h_gpu Float64,
        vm_cost_lifetime Float64,
        vm_cost_1h Float64,
        vm_cost_4h Float64,
        ba_az_count UInt64,
        vm_type String,
        vm_cpu_load_avg_avg Float64,
        vm_cpu_load_avg_max Float64,
        vm_cpu_load_avg_median Float64,
        vm_cpu_load_avg_min Float64,
        vm_cpu_load_max Float64,
        vm_cpu_load_min Float64
        
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(vm_id,vm_start) PARTITION BY toYYYYMM(slice_time)
