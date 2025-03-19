base:
    '*':
        - common
    'vm-image-template.db.yandex.net':
        - mdb_dbaas_templates
    'vm-image-template-test.db.yandex.net':
        - mdb_dbaas_templates
    'vm-image-template.yadc.io':
        - mdb_dbaas_templates
    'vm-image-template.at.double.cloud':
        - mdb_dbaas_templates
    'vm-image-template-windows-test.db.yandex.net':
        - hosts.vm_image_template_windows_test_db_yandex_net
    'ya:conductor:mdb_review':
        - match: grain
        - mdb_review
    'ya:conductor:mdb_ci':
        - match: grain
        - mdb_ci
    'ya:conductor:mdb_dbm_test':
        - match: grain
        - mdb_dbm_test
    'ya:conductor:mdb_ui_porto_test':
        - match: grain
        - mdb_controlplane_porto_test.mdb_ui_test
    'ya:conductor:mdb_ui_compute_preprod':
        - match: grain
        - mdb_controlplane_compute_preprod.mdb_ui_preprod
    'zk-dbaas-preprod01f.cloud-preprod.yandex.net,zk-dbaas-preprod01h.cloud-preprod.yandex.net,zk-dbaas-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_zk_preprod
    'mdb-report-preprod01f.cloud-preprod.yandex.net,mdb-report-preprod01h.cloud-preprod.yandex.net,mdb-report-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.report
    'meta-dbaas-preprod01f.cloud-preprod.yandex.net,meta-dbaas-preprod01h.cloud-preprod.yandex.net,meta-dbaas-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_meta_preprod
    'api-admin-dbaas-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_api_admin_preprod
    'worker-dbaas-preprod01f.cloud-preprod.yandex.net,worker-dbaas-preprod01h.cloud-preprod.yandex.net,worker-dbaas-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_worker_preprod
    'health-dbaas-preprod01f.cloud-preprod.yandex.net,health-dbaas-preprod01h.cloud-preprod.yandex.net,health-dbaas-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_health_preprod
    'dbaas-e2e-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_e2e_preprod
    'mdb-dns-preprod01f.cloud-preprod.yandex.net,mdb-dns-preprod01h.cloud-preprod.yandex.net,mdb-dns-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_dns_preprod
    'dataproc-ui-proxy-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_dataproc_ui_proxy_preprod
    'dataproc-ui-proxy01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_dataproc_ui_proxy_prod
    'mlockdb-preprod01f.cloud-preprod.yandex.net,mlockdb-preprod01h.cloud-preprod.yandex.net,mlockdb-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_mlockdb_preprod
    'mlock01f.yandexcloud.net,mlock01h.yandexcloud.net,mlock01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_mlock_prod
    'mlockdb01f.yandexcloud.net,mlockdb01h.yandexcloud.net,mlockdb01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_mlockdb_prod
    'ya:conductor:mdb_ui_compute_prod':
        - match: grain
        - mdb_controlplane_compute_prod.mdb_ui_prod
    'mdb-dns01f.yandexcloud.net,mdb-dns01h.yandexcloud.net,mdb-dns01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_dns_prod
    'zk-dbaas01f.yandexcloud.net,zk-dbaas01h.yandexcloud.net,zk-dbaas01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_zk01_prod
    'zk-dbaas02f.yandexcloud.net,zk-dbaas02h.yandexcloud.net,zk-dbaas02k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_zk02_prod
    'mdb-report01f.yandexcloud.net,mdb-report01h.yandexcloud.net,mdb-report01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_report_prod
    'meta-dbaas01f.yandexcloud.net,meta-dbaas01h.yandexcloud.net,meta-dbaas01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_meta_prod
    'api-dbaas01f.yandexcloud.net,api-dbaas02f.yandexcloud.net,api-dbaas01h.yandexcloud.net,api-dbaas02h.yandexcloud.net,api-dbaas01k.yandexcloud.net,api-dbaas02k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_api_prod
    'api-admin-dbaas01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_api_admin_prod
    'worker-dbaas01f.yandexcloud.net,worker-dbaas01h.yandexcloud.net,worker-dbaas01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_workers_prod
    'health-dbaas01f.yandexcloud.net,health-dbaas01h.yandexcloud.net,health-dbaas01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_health_prod
    'dataproc-manager01f.yandexcloud.net,dataproc-manager01h.yandexcloud.net,dataproc-manager01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_dataproc_manager_prod
    'dbaas-e2e01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_e2e_prod
    'ya:conductor:mdb_s3_zk_compute_preprod':
        - match: grain
        - mdb_s3_compute_preprod.mdb_s3_zk_compute_preprod
    'ya:conductor:mdb_pgmeta_compute_preprod':
        - match: grain
        - mdb_s3_compute_preprod.mdb_pgmeta_compute_preprod
    'ya:conductor:mdb_s3meta_compute_preprod':
        - match: grain
        - mdb_s3_compute_preprod.mdb_s3meta_compute_preprod
    'ya:conductor:mdb_s3db_compute_preprod':
        - match: grain
        - mdb_s3_compute_preprod.mdb_s3db_compute_preprod
    'ya:conductor:mdb_s3_zk_compute_prod':
        - match: grain
        - mdb_s3_compute_prod.mdb_s3_zk_compute_prod
    'ya:conductor:mdb_pgmeta_compute_prod':
        - match: grain
        - mdb_s3_compute_prod.mdb_pgmeta_compute_prod
    'ya:conductor:mdb_s3meta_compute_prod':
        - match: grain
        - mdb_s3_compute_prod.mdb_s3meta_compute_prod
    'ya:conductor:mdb_s3db_compute_prod':
        - match: grain
        - mdb_s3_compute_prod.mdb_s3db_compute_prod
    'bionic-test01i.mail.yandex.net':
        - match: compound
        - hosts.bionic-test01i_mail_yandex_net
    'ya:conductor:mail_pgtest_pgmeta':
        - match: grain
        - hosts.pgmeta-test_mail_yandex_net
    'ya:conductor:mdb_idm_service':
        - match: grain
        - mdb_idm_service
    'ya:conductor:mdb_idm_test':
        - match: grain
        - mdb_idm_test
    'x4mmm-dev01f.db.yandex.net':
        - hosts.x4mmm_dev01f_db_yandex_net
    'x4mmm-bench01i.db.yandex.net':
        - hosts.x4mmm_dev01f_db_yandex_net
    'redis-test*.db.yandex.net':
        - mdb_redis
    'ya:conductor:mdb_integration_test':
        - match: grain
        - mdb_integration_test
    'ya:conductor:mdb_sentry':
        - match: grain
        - mdb_sentry
    'dbaas-e2e01k.db.yandex.net':
        - hosts.dbaas-e2e01k_db_yandex_net
    'ya:conductor:mail_pg_common_test':
        - match: grain
        - mail_pg_common_test
    'ya:conductor:mdb_s3meta_porto_test':
        - match: grain
        - mdb_s3_porto_test.mdb_s3meta_porto_test
    'ya:conductor:mdb_s3db_porto_test':
        - match: grain
        - mdb_s3_porto_test.mdb_s3db_porto_test
    'ya:conductor:mail_pgproxy_common_test':
        - match: grain
        - mail_pgproxy_common_test
    'meta01k.db.yandex.net,meta01f.db.yandex.net,meta01h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_meta_prod
    'ya:conductor:mail_pgproxy_loadtest':
        - match: grain
        - mail_pgproxy_loadtest
    'pgmeta-load01h.mail.yandex.net':
        - hosts.pgmeta-load01h_mail_yandex_net
    'ya:conductor:mail_zk_mops':
        - match: grain
        - mail_zk.mail_zk_mops
    'ya:conductor:mail_zk_test':
        - match: grain
        - mail_zk.mail_zk_test
    'backup03h.mail.yandex.net':
        - hosts.backup03h_mail_yandex_net
    'ya:conductor:mail_pgmeta':
        - match: grain
        - mail_pgmeta
    'G@ya:conductor:mail_pgproxy':
        - match: compound
        - mail_pgproxy
    'backup04h.mail.yandex.net':
        - hosts.backup04h_mail_yandex_net
    'ya:conductor:mdb_backup':
        - match: grain
        - mdb_backup
    'ya:conductor:mdb_backup_test':
        - match: grain
        - mdb_backup_test
    'ya:conductor:mail_logstore':
        - match: grain
        - mail_logstore
    'ya:conductor:mail_mdb_srv_test':
        - match: grain
        - mail_mdb_srv_test
    'ya:conductor:mail_mdb_srv':
        - match: grain
        - mail_mdb_srv
    'ya:conductor:mdb_dom0':
        - match: grain
        - mdb_dom0
    'ya:conductor:mdb_dom0_test':
        - match: grain
        - mdb_dom0_test
    'ya:conductor:mdb_s3meta_porto_prod':
        - match: grain
        - mdb_s3_porto_prod.mdb_s3meta_porto_prod
    'ya:conductor:mail_s3db':
        - match: grain
        - mail_s3db
    'ya:conductor:mdb_s3db_porto_prod':
        - match: grain
        - mail_s3db
    'ya:conductor:mdb_s3meta_blue_porto_prod':
        - match: grain
        - mdb_s3_porto_prod.mdb_s3meta_blue_porto_prod
    'ya:conductor:mdb_s3db_blue_porto_prod':
        - match: grain
        - mdb_s3_porto_prod.mdb_s3db_blue_porto_prod
    'ya:conductor:mdb_pgmeta_blue_porto_prod':
        - match: grain
        - mdb_s3_porto_prod.mdb_pgmeta_blue_porto_prod
    'ya:conductor:mdb_pgmeta_porto_prod':
        - match: grain
        - mdb_s3_porto_prod.mdb_pgmeta_porto_prod
    'wal-g-test-db*':
        - hosts.wal-g-test-db
    'mialinx01f*':
        - hosts.mialinx01f_mail_yandex_net
    'mialinx01h*':
        - hosts.mialinx01h_mail_yandex_net
    'mialinx01i*':
        - hosts.mialinx01i_mail_yandex_net
    'ya:conductor:mdb_zkeeper':
        - match: grain
        - mdb_zkeeper
    'ya:conductor:mdb_zkeeper02':
        - match: grain
        - mdb_zkeeper02
    'ya:conductor:mdb_zkeeper03':
        - match: grain
        - mdb_zkeeper03
    'ya:conductor:mdb_zkeeper04':
        - match: grain
        - mdb_zkeeper04
    'ya:conductor:mdb_zkeeper05':
        - match: grain
        - mdb_zkeeper05
    'ya:conductor:mdb_zkeeper_test':
        - match: grain
        - mdb_zkeeper_test
    'ya:conductor:mdb_zkeeper_test02':
        - match: grain
        - mdb_zkeeper_test02
    'ya:conductor:mdb_zk_df_e2e':
        - match: grain
        - mdb_zk_df_e2e
    'dbm01h.db.yandex.net,dbm01k.db.yandex.net,dbm01f.db.yandex.net,dbm01e.db.yandex.net':
        - match: list
        - mdb_dbm
    'dbmdb01h.db.yandex.net,dbmdb01f.db.yandex.net,dbmdb01k.db.yandex.net':
        - match: list
        - mdb_dbmdb
    'ya:conductor:mdb_ui_porto_prod':
        - match: grain
        - mdb_controlplane_porto_prod.mdb_ui_prod
    'internal-api01e.db.yandex.net,internal-api01f.db.yandex.net,internal-api01h.db.yandex.net,internal-api01k.db.yandex.net,internal-api02e.db.yandex.net,internal-api02f.db.yandex.net,internal-api02h.db.yandex.net,internal-api02k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_api_prod
    'api-admin-dbaas01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_api_admin_prod
    'dbaas-worker01e.db.yandex.net,dbaas-worker01f.db.yandex.net,dbaas-worker01h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_worker_prod
    'odyssey-test0*i.mail.yandex.net':
        - hosts.odyssey-test01i_mail_yandex_net
    'mdb-internal-api-test01h.db.yandex.net,mdb-internal-api-test01k.db.yandex.net,mdb-internal-api-test01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_internal_api_test
    'mdb-internal-api01h.db.yandex.net,mdb-internal-api01k.db.yandex.net,mdb-internal-api01f.db.yandex.net,mdb-internal-api01e.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_internal_api_prod
    'mdb-internal-api-preprod01h.cloud-preprod.yandex.net,mdb-internal-api-preprod01k.cloud-preprod.yandex.net,mdb-internal-api-preprod01f.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_internal_api_preprod
    'mdb-internal-api01h.yandexcloud.net,mdb-internal-api01k.yandexcloud.net,mdb-internal-api01f.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_internal_api_prod
    'health-test01h.db.yandex.net,health-test01f.db.yandex.net,health-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_health_test
    'health01h.db.yandex.net,health01f.db.yandex.net,health01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_health_prod
    'mdb-dns-test01h.db.yandex.net,mdb-dns-test01f.db.yandex.net,mdb-dns-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_dns_porto_test
    'mdb-dns01h.db.yandex.net,mdb-dns01e.db.yandex.net,mdb-dns01k.db.yandex.net,mdb-dns01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_dns_prod
    'deploy-db01k.db.yandex.net,deploy-db01h.db.yandex.net,deploy-db01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_deploy_db_prod
    'ya:conductor:mdb_deploy_salt_personal_porto_prod':
        - match: grain
        - mdb_controlplane_porto_prod.mdb_deploy_salt_prod_personal
    'deploy-salt01k.db.yandex.net,deploy-salt01h.db.yandex.net,deploy-salt01f.db.yandex.net,deploy-salt02h.db.yandex.net,deploy-salt02k.db.yandex.net,deploy-salt02f.db.yandex.net,deploy-salt03h.db.yandex.net,deploy-salt03k.db.yandex.net,deploy-salt03f.db.yandex.net,deploy-salt04h.db.yandex.net,deploy-salt04k.db.yandex.net,deploy-salt04f.db.yandex.net,deploy-salt05h.db.yandex.net,deploy-salt05k.db.yandex.net,deploy-salt05f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_deploy_salt_prod
    'deploy-api01k.db.yandex.net,deploy-api01h.db.yandex.net,deploy-api01f.db.yandex.net,deploy-api01e.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_deploy_api_prod
    'mdb-salt-sync01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.salt_sync_prod
    'deploy-db-test01h.db.yandex.net,deploy-db-test01f.db.yandex.net,deploy-db-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_deploy_db_test
    'deploy-salt-test01h.db.yandex.net,deploy-salt-test01k.db.yandex.net,deploy-salt-test01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.deploy_salt
    'ya:conductor:mdb_deploy_salt_personal_porto_test':
        - match: grain
        - mdb_controlplane_porto_test.deploy_salt.personal
    'deploy-api-test01h.db.yandex.net,deploy-api-test01f.db.yandex.net,deploy-api-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_deploy_api_test
    'mdb-secrets-test01f.db.yandex.net,mdb-secrets-test01k.db.yandex.net,mdb-secrets-test01h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_secrets_api_test
    'mdb-secrets01k.db.yandex.net,mdb-secrets01h.db.yandex.net,mdb-secrets01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_secrets_api_prod
    'secrets-db-test01h.db.yandex.net,secrets-db-test01f.db.yandex.net,secrets-db-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_secrets_db_test
    'secrets-db01k.db.yandex.net,secrets-db01h.db.yandex.net,secrets-db01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_secrets_db_prod
    'mdb-deploy-db-preprod01f.cloud-preprod.yandex.net,mdb-deploy-db-preprod01h.cloud-preprod.yandex.net,mdb-deploy-db-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_deploy_db_preprod
    'mdb-deploy-salt-preprod01h.cloud-preprod.yandex.net,mdb-deploy-salt-preprod01k.cloud-preprod.yandex.net,mdb-deploy-salt-preprod01f.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.deploy_salt
    'mdb-deploy-salt-d0uble01k.cloud-preprod.yandex.net,mdb-deploy-salt-common01f.cloud-preprod.yandex.net,mdb-deploy-salt-common01h.cloud-preprod.yandex.net,mdb-deploy-salt-common01k.cloud-preprod.yandex.net,mdb-deploy-salt-arhipov01k.cloud-preprod.yandex.net,mdb-salt-personal-velom-preprod01-rc1a.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.deploy_salt.personal
    'mdb-deploy-db01f.yandexcloud.net,mdb-deploy-db01h.yandexcloud.net,mdb-deploy-db01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_deploy_db_prod
    'katan-db-preprod01f.cloud-preprod.yandex.net,katan-db-preprod01h.cloud-preprod.yandex.net,katan-db-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_katan_db_preprod
    'katan-db01k.db.yandex.net,katan-db01h.db.yandex.net,katan-db01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_katan_db_prod
    'mdb-deploy-salt01f.yandexcloud.net,mdb-deploy-salt01h.yandexcloud.net,mdb-deploy-salt01k.yandexcloud.net,mdb-deploy-salt02f.yandexcloud.net,mdb-deploy-salt02h.yandexcloud.net,mdb-deploy-salt02k.yandexcloud.net,mdb-deploy-salt03f.yandexcloud.net,mdb-deploy-salt03h.yandexcloud.net,mdb-deploy-salt03k.yandexcloud.net,mdb-deploy-salt04f.yandexcloud.net,mdb-deploy-salt04h.yandexcloud.net,mdb-deploy-salt04k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_deploy_salt_prod
    'mdb-deploy-api01f.yandexcloud.net,mdb-deploy-api01h.yandexcloud.net,mdb-deploy-api01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_deploy_api_prod
    'dbaas-worker-test01h.db.yandex.net,dbaas-worker-test01f.db.yandex.net,dbaas-worker-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_worker_test
    'internal-api-test01h.db.yandex.net,internal-api-test01f.db.yandex.net,internal-api-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_api_test
    'api-admin-dbaas-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_api_admin_test
    'meta-test01k.db.yandex.net,meta-test01f.db.yandex.net,meta-test01h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_meta_test
    'secrets-db-preprod01f.cloud-preprod.yandex.net,secrets-db-preprod01h.cloud-preprod.yandex.net,secrets-db-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_secrets_db_preprod
    'secrets-db-prod01f.yandexcloud.net,secrets-db-prod01k.yandexcloud.net,secrets-db-prod01h.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_secrets_db_prod
    'mdb-secrets-prod01f.yandexcloud.net,mdb-secrets-prod01k.yandexcloud.net,mdb-secrets-prod01h.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_secrets_api_prod
    'mdb-report-test-01e.db.yandex.net,mdb-report-test-01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.report
    'mdb-report-01f.db.yandex.net,mdb-report-01h.db.yandex.net,mdb-report-01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_report_porto_prod
    'katan-db-test01f.db.yandex.net,katan-db-test01k.db.yandex.net,katan-db-test01h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_katan_db_test
    'cms-api-test01h.db.yandex.net,cms-api-test01f.db.yandex.net,cms-api-test02h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_cms_api_test
    'cms-db-test01h.db.yandex.net,cms-db-test01k.db.yandex.net,cms-db-test01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_cms_db_test
    'cms-grpcapi-test01h.db.yandex.net,cms-grpcapi-test01f.db.yandex.net,cms-grpcapi-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_cms_grpcapi_test
    'cms-instance-autoduty-test01h.db.yandex.net,cms-instance-autoduty-test01f.db.yandex.net,cms-instance-autoduty-test01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_cms_autoduty_test
    'cms-db-prod01h.db.yandex.net,cms-db-prod01k.db.yandex.net,cms-db-prod01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_cms_db_prod
    'cms-api-prod01h.db.yandex.net,cms-api-prod01f.db.yandex.net,cms-api-prod01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_cms_api_prod
    'cms-grpcapi-prod01h.db.yandex.net,cms-grpcapi-prod01f.db.yandex.net,cms-grpcapi-prod01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_cms_grpcapi_prod
    'cms-instance-autoduty-prod01h.db.yandex.net,cms-instance-autoduty-prod01f.db.yandex.net,cms-instance-autoduty-prod01k.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_cms_autoduty_prod
    'mdb-cmsgrpcapi-preprod01-rc1a.cloud-preprod.yandex.net,mdb-cmsgrpcapi-preprod01-rc1b.cloud-preprod.yandex.net,mdb-cmsgrpcapi-preprod01-rc1c.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_cms_grpcapi_preprod
    'mdb-cmsdb-preprod01-rc1a.cloud-preprod.yandex.net,mdb-cmsdb-preprod01-rc1b.cloud-preprod.yandex.net,mdb-cmsdb-preprod01-rc1c.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_cms_db_preprod
    'mdb-cms-autoduty-preprod01-rc1a.cloud-preprod.yandex.net,mdb-cms-autoduty-preprod01-rc1b.cloud-preprod.yandex.net,mdb-cms-autoduty-preprod01-rc1c.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_cms_autoduty_preprod
    'mdb-cmsgrpcapi01-rc1a.yandexcloud.net,mdb-cmsgrpcapi01-rc1b.yandexcloud.net,mdb-cmsgrpcapi01-rc1c.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_cms_grpcapi_prod
    'mdb-cmsdb01-rc1a.yandexcloud.net,mdb-cmsdb01-rc1b.yandexcloud.net,mdb-cmsdb01-rc1c.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_cms_db_prod
    'mdb-cms-autoduty01-rc1a.yandexcloud.net,mdb-cms-autoduty01-rc1b.yandexcloud.net,mdb-cms-autoduty01-rc1c.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_cms_autoduty_prod
    'mlockdb-test01k.db.yandex.net,mlockdb-test01h.db.yandex.net,mlockdb-test01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_mlockdb_test
    'mlock-test01k.db.yandex.net,mlock-test01h.db.yandex.net,mlock-test01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_mlock_test
    'mlockdb01k.db.yandex.net,mlockdb01h.db.yandex.net,mlockdb01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_mlockdb_prod
    'mlock01k.db.yandex.net,mlock01h.db.yandex.net,mlock01f.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_mlock_prod
    'mdb-bootstrap01h.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_appendix_salt_2if
    'mdb-bootstrap01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_appendix_salt
    'mdb-katandb01-rc1a.yandexcloud.net,mdb-katandb01-rc1b.yandexcloud.net,mdb-katandb01-rc1c.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_katandb_prod
    'backup-test01h.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_test.mdb_backup_porto_test
    'mdb-backup01-myt.db.yandex.net,mdb-backup01-sas.db.yandex.net,mdb-backup01-vla.db.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.mdb_backup_porto_prod
    'mdb-backup-preprod01-rc1a.cloud-preprod.yandex.net,mdb-backup-preprod01-rc1b.cloud-preprod.yandex.net,mdb-backup-preprod01-rc1c.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_backup_compute_preprod
    'mdb-backup01-rc1a.yandexcloud.net,mdb-backup01-rc1b.yandexcloud.net,mdb-backup01-rc1c.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_backup_compute_prod
    'mdb-wsus-preprod01f.cloud-preprod.yandex.net,mdb-wsus-preprod01h.cloud-preprod.yandex.net,mdb-wsus-preprod01k.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_wsus_compute_preprod
    'mdb-wsus-prod01f.yandexcloud.net,mdb-wsus-prod01h.yandexcloud.net,mdb-wsus-prod01k.yandexcloud.net':
        - match: list
        - mdb_controlplane_compute_prod.mdb_wsus_compute_prod
    'datacloud-dist-sync.sas.yp-c.yandex.net':
        - match: list
        - mdb_controlplane_porto_prod.datacloud_dist_sync
    'mdb-samba-preprod01-rc1a.cloud-preprod.yandex.net,mdb-samba-preprod01-rc1b.cloud-preprod.yandex.net,mdb-samba-preprod01-rc1c.cloud-preprod.yandex.net':
        - match: list
        - mdb_controlplane_compute_preprod.mdb_samba_compute_preprod
    'tank-common01h.db.yandex.net':
        - match: list
        - porto.prod.tanks
    'tank-test01h.db.yandex.net':
        - match: list
        - porto.test.tanks
    'tank-common01h.yandexcloud.net':
        - match: list
        - compute.prod.tanks
    'zk01-01-il1-a.mdb-s3.yandexcloud.co.il,zk01-02-il1-a.mdb-s3.yandexcloud.co.il,zk01-03-il1-a.mdb-s3.yandexcloud.co.il':
        - match: list
        - mdb_s3_israel.mdb_s3_zk_israel_prod
    's3meta01-01-il1-a.mdb-s3.yandexcloud.co.il,s3meta01-02-il1-a.mdb-s3.yandexcloud.co.il,s3meta01-03-il1-a.mdb-s3.yandexcloud.co.il':
        - match: list
        - mdb_s3_israel.mdb_s3meta_israel_prod
        - mdb_s3_israel.cert
    'pgmeta01-01-il1-a.mdb-s3.yandexcloud.co.il,pgmeta01-02-il1-a.mdb-s3.yandexcloud.co.il':
        - match: list
        - mdb_s3_israel.mdb_pgmeta_israel_prod
        - mdb_s3_israel.cert
    's3db01-01-il1-a.mdb-s3.yandexcloud.co.il,s3db01-02-il1-a.mdb-s3.yandexcloud.co.il,s3db01-03-il1-a.mdb-s3.yandexcloud.co.il':
        - match: list
        - mdb_s3_israel.mdb_s3db_israel_prod
        - mdb_s3_israel.s3db_shards.01
        - mdb_s3_israel.cert
    'zk01-*.mdb-cp.yandexcloud.co.il':
        - mdb_controlplane_israel.zk01
    'secretsdb*.mdb-cp.yandexcloud.co.il':
        - mdb_controlplane_israel.secretsdb
    'metadb*.mdb-cp.yandexcloud.co.il':
        - mdb_controlplane_israel.metadb
    'deploydb*.mdb-cp.yandexcloud.co.il':
        - mdb_controlplane_israel.deploydb
    'deploy-salt*.mdb-cp.yandexcloud.co.il':
        - mdb_controlplane_israel.deploy_salt
