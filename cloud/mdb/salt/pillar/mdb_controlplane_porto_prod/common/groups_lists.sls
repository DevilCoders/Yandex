data:
    mdb_cms:
        cms:
            groups_black_list:
                - mdb_deploy_salt_personal_porto_test
                - mdb_deploy_salt_personal_porto_prod
                - datacloud_dist_sync
                - mdb_greenplum_master_porto_test
                - mdb_greenplum_segment_porto_test
                - mdb_opsm_porto_prod
                - mdb_blacklist
                - mdb_zk_df_e2e
                - mdb_tanks_porto_prod
                - mdb_tanks_porto_test
                - mdb_backup
                - mdb_backup_test
                - mdb_e2e_porto_test
            groups_white_list:
                -   type: dataplane
                    groups:
                        - mdb_dataplane_porto_test
                        - mdb_dataplane_porto_prod
                -   type: dataproc
                    groups:
                        - mdb_dataproc_infratest
                -   type: s3
                    max_unhealthy: 1
                    checks:
                        - META
                        - bouncer_ping
                        - pg_ping
                        - pg_replication_alive
                        - pg_replication_lag
                        - s3_closer
                    groups:
                        - mdb_s3db_porto_test
                        - mdb_s3meta_porto_test
                        - mdb_s3db_porto_prod
                        - mdb_s3meta_porto_prod
                -   type: unsorted testing groups
                    groups:
                        - mail_pgtest
                        - dbaas_infra_test_vms
                        - mdb_controlplane_integration_test
                        - mail_pgproxy_loadtest
                        - mdb_idm_test
                -   type: control plane api
                    max_unhealthy: 1
                    checks:
                        - META
                        - UNREACHABLE
                    groups:
                        - mdb_idm_service
                        - mdb_worker_porto_prod
                        - mdb_api_porto_prod
                        - mdb_health_porto_prod
                        - mdb_api_admin_porto_prod
                        - mdb_dns_porto_prod
                        - mdb_secrets_api_porto_prod
                        - mdb_report_porto_prod
                        - mdb_deploy_salt_porto_prod
                        - mdb_deploy_api_porto_prod
                        - mdb_internal_api_porto_prod
                        - mdb_cms_api_porto_prod
                        - mdb_cms_grpcapi_porto_prod
                        - mdb_cms_autoduty_porto_prod
                        - mdb_dbm
                        - mdb_dbm_test
                        - mdb_mlock_porto_prod
                        - mdb_ui_porto_prod
                        - mdb_backup_porto_prod
                        - mdb_salt_sync_porto_prod
                        - mdb_secrets_api_porto_prod
                        - mdb_deploy_api_porto_test
                        - mdb_deploy_salt_porto_test
                        - mdb_deploy_db_porto_test
                        - mdb_deploy_porto_test
                        - mdb_secrets_porto_test
                        - mdb_health_porto_test
                        - mdb_report_porto_test
                        - mdb_katan_porto_test
                        - mdb_internal_api_porto_test
                        - mdb_cms_api_porto_test
                        - mdb_cms_autoduty_porto_test
                        - mdb_cms_grpcapi_porto_test
                        - mdb_mlock_porto_test
                        - mdb_worker_porto_test
                        - mdb_api_porto_test
                        - mdb_api_admin_porto_test
                        - mdb_ui_porto_test
                        - mdb_dns_porto_test
                        - mdb_backup_porto_test
                        - mdb_sentry
                -   type: control plane databases
                    max_unhealthy: 1
                    checks:
                        - META
                        - bouncer_ping
                        - pg_ping
                        - pg_replication_alive
                        - pg_replication_lag
                        - pg_xlog_files
                    groups:
                        - mdb_cms_db_porto_prod
                        - mdb_cmsdb_porto_test
                        - mdb_dbmdb
                        - mdb_dbm_test
                        - mdb_katan_db_porto_prod
                        - mdb_katan_db_porto_test
                        - mdb_secrets_db_porto_prod
                        - mdb_secrets_db_porto_test
                        - mdb_meta_porto_prod
                        - mdb_meta_porto_test
                        - mdb_mlockdb_porto_prod
                        - mdb_mlockdb_porto_test
                        - mdb_deploy_db_porto_prod
                        - mdb_deploy_db_porto_test
                -   type: zookeeper
                    max_unhealthy: 1
                    checks:
                        - META
                        - UNREACHABLE
                        - zk_alive
                    groups:
                        - mdb_zkeepers
                        - mdb_zkeeper_test
