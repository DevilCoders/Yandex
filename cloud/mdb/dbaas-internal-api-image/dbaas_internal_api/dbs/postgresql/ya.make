OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

SET(PACKAGE_NAME dbaas_internal_api.dbs.postgresql)

PY_SRCS(
    NAMESPACE ${PACKAGE_NAME}
    __init__.py
    db.py
    node_pool.py
    pool.py
    query.py
)

SET(QS_DIR query_conf)

SET(RES_PREFIX ${PACKAGE_NAME}.query)

RESOURCE(
    ${QS_DIR}/add_backup_schedule.sql ${RES_PREFIX}/add_backup_schedule.sql
    ${QS_DIR}/add_cluster.sql ${RES_PREFIX}/add_cluster.sql
    ${QS_DIR}/add_finished_operation.sql ${RES_PREFIX}/add_finished_operation.sql
    ${QS_DIR}/add_finished_operation_for_current_rev.sql ${RES_PREFIX}/add_finished_operation_for_current_rev.sql
    ${QS_DIR}/add_hadoop_job.sql ${RES_PREFIX}/add_hadoop_job.sql
    ${QS_DIR}/add_host.sql ${RES_PREFIX}/add_host.sql
    ${QS_DIR}/add_operation.sql ${RES_PREFIX}/add_operation.sql
    ${QS_DIR}/add_pillar.sql ${RES_PREFIX}/add_pillar.sql
    ${QS_DIR}/add_shard.sql ${RES_PREFIX}/add_shard.sql
    ${QS_DIR}/add_subcluster.sql ${RES_PREFIX}/add_subcluster.sql
    ${QS_DIR}/add_alert_group.sql ${RES_PREFIX}/add_alert_group.sql
    ${QS_DIR}/add_target_pillar.sql ${RES_PREFIX}/add_target_pillar.sql
    ${QS_DIR}/add_to_search_queue.sql ${RES_PREFIX}/add_to_search_queue.sql
    ${QS_DIR}/add_to_worker_queue_events.sql ${RES_PREFIX}/add_to_worker_queue_events.sql
    ${QS_DIR}/add_unmanaged_operation.sql ${RES_PREFIX}/add_unmanaged_operation.sql
    ${QS_DIR}/cluster_is_managed.sql ${RES_PREFIX}/cluster_is_managed.sql
    ${QS_DIR}/complete_cluster_change.sql ${RES_PREFIX}/complete_cluster_change.sql
    ${QS_DIR}/config_host_auth.sql ${RES_PREFIX}/config_host_auth.sql
    ${QS_DIR}/create_cloud.sql ${RES_PREFIX}/create_cloud.sql
    ${QS_DIR}/create_folder.sql ${RES_PREFIX}/create_folder.sql
    ${QS_DIR}/add_instance_group_subcluster.sql ${RES_PREFIX}/add_instance_group_subcluster.sql
    ${QS_DIR}/delete_host.sql ${RES_PREFIX}/delete_host.sql
    ${QS_DIR}/delete_shard.sql ${RES_PREFIX}/delete_shard.sql
    ${QS_DIR}/delete_subcluster.sql ${RES_PREFIX}/delete_subcluster.sql
    ${QS_DIR}/delete_alert_group.sql ${RES_PREFIX}/delete_alert_group.sql
    ${QS_DIR}/delete_alert_from_group.sql ${RES_PREFIX}/delete_alert_from_group.sql
    ${QS_DIR}/set_default_versions.sql ${RES_PREFIX}/set_default_versions.sql
    ${QS_DIR}/finish_unmanaged_task.sql ${RES_PREFIX}/finish_unmanaged_task.sql
    ${QS_DIR}/get_all_geo.sql ${RES_PREFIX}/get_all_geo.sql
    ${QS_DIR}/get_cid_by_host_attrs.sql ${RES_PREFIX}/get_cid_by_host_attrs.sql
    ${QS_DIR}/get_cloud.sql ${RES_PREFIX}/get_cloud.sql
    ${QS_DIR}/get_cluster.sql ${RES_PREFIX}/get_cluster.sql
    ${QS_DIR}/get_cluster_at_rev.sql ${RES_PREFIX}/get_cluster_at_rev.sql
    ${QS_DIR}/get_cluster_by_managed_host.sql ${RES_PREFIX}/get_cluster_by_managed_host.sql
    ${QS_DIR}/get_cluster_by_pillar_value.sql ${RES_PREFIX}/get_cluster_by_pillar_value.sql
    ${QS_DIR}/get_cluster_by_unmanaged_host.sql ${RES_PREFIX}/get_cluster_by_unmanaged_host.sql
    ${QS_DIR}/get_cluster_quota_usage.sql ${RES_PREFIX}/get_cluster_quota_usage.sql
    ${QS_DIR}/get_cluster_rev_by_timestamp.sql ${RES_PREFIX}/get_cluster_rev_by_timestamp.sql
    ${QS_DIR}/get_cluster_type_pillar.sql ${RES_PREFIX}/get_cluster_type_pillar.sql
    ${QS_DIR}/get_default_versions.sql ${RES_PREFIX}/get_default_versions.sql
    ${QS_DIR}/get_cluster_version_at_rev.sql ${RES_PREFIX}/get_cluster_version_at_rev.sql
    ${QS_DIR}/get_clusters_versions.sql ${RES_PREFIX}/get_clusters_versions.sql
    ${QS_DIR}/get_subclusters_versions.sql ${RES_PREFIX}/get_subclusters_versions.sql
    ${QS_DIR}/get_clusters_by_folder.sql ${RES_PREFIX}/get_clusters_by_folder.sql
    ${QS_DIR}/get_cluster_by_ag.sql ${RES_PREFIX}/get_cluster_by_ag.sql
    ${QS_DIR}/get_clusters_change_dates.sql ${RES_PREFIX}/get_clusters_change_dates.sql
    ${QS_DIR}/get_clusters_count_in_folder.sql ${RES_PREFIX}/get_clusters_count_in_folder.sql
    ${QS_DIR}/get_clusters_essence_by_folder.sql ${RES_PREFIX}/get_clusters_essence_by_folder.sql
    ${QS_DIR}/get_custom_pillar_by_host.sql ${RES_PREFIX}/get_custom_pillar_by_host.sql
    ${QS_DIR}/get_default_feature_flags.sql ${RES_PREFIX}/get_default_feature_flags.sql
    ${QS_DIR}/get_disk_types.sql ${RES_PREFIX}/get_disk_types.sql
    ${QS_DIR}/get_flavor_by_id.sql ${RES_PREFIX}/get_flavor_by_id.sql
    ${QS_DIR}/get_flavor_by_name.sql ${RES_PREFIX}/get_flavor_by_name.sql
    ${QS_DIR}/get_flavors.sql ${RES_PREFIX}/get_flavors.sql
    ${QS_DIR}/get_folder.sql ${RES_PREFIX}/get_folder.sql
    ${QS_DIR}/get_folder_by_cluster.sql ${RES_PREFIX}/get_folder_by_cluster.sql
    ${QS_DIR}/get_folder_by_task.sql ${RES_PREFIX}/get_folder_by_task.sql
    ${QS_DIR}/get_folders_by_cloud.sql ${RES_PREFIX}/get_folders_by_cloud.sql
    ${QS_DIR}/get_fqdn_pillar.sql ${RES_PREFIX}/get_fqdn_pillar.sql
    ${QS_DIR}/get_hadoop_job.sql ${RES_PREFIX}/get_hadoop_job.sql
    ${QS_DIR}/get_hadoop_job_task.sql ${RES_PREFIX}/get_hadoop_job_task.sql
    ${QS_DIR}/get_hadoop_jobs_by_cluster.sql ${RES_PREFIX}/get_hadoop_jobs_by_cluster.sql
    ${QS_DIR}/get_hadoop_jobs_by_service.sql ${RES_PREFIX}/get_hadoop_jobs_by_service.sql
    ${QS_DIR}/get_host_info.sql ${RES_PREFIX}/get_host_info.sql
    ${QS_DIR}/get_hosts.sql ${RES_PREFIX}/get_hosts.sql
    ${QS_DIR}/get_hosts_at_rev.sql ${RES_PREFIX}/get_hosts_at_rev.sql
    ${QS_DIR}/get_hosts_by_shard.sql ${RES_PREFIX}/get_hosts_by_shard.sql
    ${QS_DIR}/get_instance_group.sql ${RES_PREFIX}/get_instance_group.sql
    ${QS_DIR}/get_instance_groups.sql ${RES_PREFIX}/get_instance_groups.sql
    ${QS_DIR}/get_oldest_shard.sql ${RES_PREFIX}/get_oldest_shard.sql
    ${QS_DIR}/get_operation_by_id.sql ${RES_PREFIX}/get_operation_by_id.sql
    ${QS_DIR}/get_operations.sql ${RES_PREFIX}/get_operations.sql
    ${QS_DIR}/get_pg_ha_hosts_by_cid.sql ${RES_PREFIX}/get_pg_ha_hosts_by_cid.sql
    ${QS_DIR}/get_pillar_by_host.sql ${RES_PREFIX}/get_pillar_by_host.sql
    ${QS_DIR}/get_resource_preset_by_cpu.sql ${RES_PREFIX}/get_resource_preset_by_cpu.sql
    ${QS_DIR}/get_resource_presets.sql ${RES_PREFIX}/get_resource_presets.sql
    ${QS_DIR}/get_resource_presets_by_cluster_type.sql ${RES_PREFIX}/get_resource_presets_by_cluster_type.sql
    ${QS_DIR}/get_shards.sql ${RES_PREFIX}/get_shards.sql
    ${QS_DIR}/get_shards_at_rev.sql ${RES_PREFIX}/get_shards_at_rev.sql
    ${QS_DIR}/get_subcluster_info.sql ${RES_PREFIX}/get_subcluster_info.sql
    ${QS_DIR}/get_subcluster_pillars_by_cluster.sql ${RES_PREFIX}/get_subcluster_pillars_by_cluster.sql
    ${QS_DIR}/get_subclusters_at_rev.sql ${RES_PREFIX}/get_subclusters_at_rev.sql
    ${QS_DIR}/get_subclusters_by_cluster.sql ${RES_PREFIX}/get_subclusters_by_cluster.sql
    ${QS_DIR}/get_subcluster_by_instance_group_id.sql ${RES_PREFIX}/get_subcluster_by_instance_group_id.sql
    ${QS_DIR}/get_task_by_idempotence_id.sql ${RES_PREFIX}/get_task_by_idempotence_id.sql
    ${QS_DIR}/get_undeleted_host.sql ${RES_PREFIX}/get_undeleted_host.sql
    ${QS_DIR}/get_undeleted_shard_hosts.sql ${RES_PREFIX}/get_undeleted_shard_hosts.sql
    ${QS_DIR}/get_undeleted_subcluster_hosts.sql ${RES_PREFIX}/get_undeleted_subcluster_hosts.sql
    ${QS_DIR}/get_valid_resources.sql ${RES_PREFIX}/get_valid_resources.sql
    ${QS_DIR}/get_zk_hosts_usage.sql ${RES_PREFIX}/get_zk_hosts_usage.sql
    ${QS_DIR}/insert_idempotence_id.sql ${RES_PREFIX}/insert_idempotence_id.sql
    ${QS_DIR}/lock_cloud.sql ${RES_PREFIX}/lock_cloud.sql
    ${QS_DIR}/lock_cluster.sql ${RES_PREFIX}/lock_cluster.sql
    ${QS_DIR}/lock_pillar.sql ${RES_PREFIX}/lock_pillar.sql
    ${QS_DIR}/pillar_insert.sql ${RES_PREFIX}/pillar_insert.sql
    ${QS_DIR}/ping.sql ${RES_PREFIX}/ping.sql
    ${QS_DIR}/reschedule_maintenance_task.sql ${RES_PREFIX}/reschedule_maintenance_task.sql
    ${QS_DIR}/set_cloud_quota.sql ${RES_PREFIX}/set_cloud_quota.sql
    ${QS_DIR}/set_labels_on_cluster.sql ${RES_PREFIX}/set_labels_on_cluster.sql
    ${QS_DIR}/set_maintenance_window_settings.sql ${RES_PREFIX}/set_maintenance_window_settings.sql
    ${QS_DIR}/update_cloud_quota.sql ${RES_PREFIX}/update_cloud_quota.sql
    ${QS_DIR}/update_cloud_used_resources.sql ${RES_PREFIX}/update_cloud_used_resources.sql
    ${QS_DIR}/update_cluster_description.sql ${RES_PREFIX}/update_cluster_description.sql
    ${QS_DIR}/update_alert.sql ${RES_PREFIX}/update_alert.sql
    ${QS_DIR}/update_cluster_deletion_protection.sql ${RES_PREFIX}/update_cluster_deletion_protection.sql
    ${QS_DIR}/update_cluster_folder.sql ${RES_PREFIX}/update_cluster_folder.sql
    ${QS_DIR}/update_cluster_name.sql ${RES_PREFIX}/update_cluster_name.sql
    ${QS_DIR}/update_hadoop_job_status.sql ${RES_PREFIX}/update_hadoop_job_status.sql
    ${QS_DIR}/update_host.sql ${RES_PREFIX}/update_host.sql
    ${QS_DIR}/update_alert_group.sql ${RES_PREFIX}/update_alert_group.sql
    ${QS_DIR}/update_pillar.sql ${RES_PREFIX}/update_pillar.sql
    ${QS_DIR}/worker_queue_get_failed_task.sql ${RES_PREFIX}/worker_queue_get_failed_task.sql
    ${QS_DIR}/worker_queue_get_running_task.sql ${RES_PREFIX}/worker_queue_get_running_task.sql
    ${QS_DIR}/terminate_hadoop_jobs.sql ${RES_PREFIX}/terminate_hadoop_jobs.sql
    ${QS_DIR}/get_managed_config.sql ${RES_PREFIX}/get_managed_config.sql
    ${QS_DIR}/get_unmanaged_config.sql ${RES_PREFIX}/get_unmanaged_config.sql
    ${QS_DIR}/get_backups.sql ${RES_PREFIX}/get_backups.sql
    ${QS_DIR}/get_alert_groups.sql ${RES_PREFIX}/get_alert_groups.sql
    ${QS_DIR}/get_managed_alert_group_by_cid.sql ${RES_PREFIX}/get_managed_alert_group_by_cid.sql
    ${QS_DIR}/get_alert_group.sql ${RES_PREFIX}/get_alert_group.sql
    ${QS_DIR}/get_ctype_alert_template.sql ${RES_PREFIX}/get_ctype_alert_template.sql
    ${QS_DIR}/get_alerts_by_alert_group.sql ${RES_PREFIX}/get_alerts_by_alert_group.sql
    ${QS_DIR}/add_alert_to_group.sql ${RES_PREFIX}/add_alert_to_group.sql
    ${QS_DIR}/schedule_backup_for_now.sql ${RES_PREFIX}/schedule_backup_for_now.sql
    ${QS_DIR}/add_default_alerts_to_alert_group.sql ${RES_PREFIX}/add_default_alerts_to_alert_group.sql
    ${QS_DIR}/get_managed_backup.sql ${RES_PREFIX}/get_managed_backup.sql
)

END()
