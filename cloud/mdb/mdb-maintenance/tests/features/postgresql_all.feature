Feature: Run all postgresql maintenance tasks
    Background: Create postgresql clusters for every config
        Given I load all configs
        And stopped postgresql cluster with name "postgresql_offline_update_tls_certs"
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        And move cluster "cid1" to "prod" env
        And I increase cloud quota
        And stopped postgresql cluster with name "postgresql_offline_update_minor_version"
        And I set "cid2" "postgres" version to "10.1"
        And move cluster "cid2" to "prod" env
        And stopped postgresql cluster with name "postgresql_offline_update_odyssey_version"
        And move cluster "cid3" to "prod" env
        And I set "cid3" "odyssey" version to "0.322"
        And postgresql cluster with "10.1" minor version and name "postgresql_enable_quorum_commit_2"
        And In cluster "cid4" I set pillar path "{data,pgsync,quorum_commit}" to bool "false"
        And postgresql cluster with "10.1" minor version and name "postgresql_enable_quorum_commit_3"
        And In cluster "cid5" I set pillar path "{data,pgsync,quorum_commit}" to bool "false"
        And postgresql cluster with name "postgresql_cluster_start_failover" and "pgbouncer" connection_pooler
        And CMS up and running with task for cluster "postgresql_cluster_start_failover"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """
        And postgresql cluster with "10.1" minor version and name "postgresql_update_odyssey_version_hotfix"
        And I set "cid7" "odyssey" version to "Odyssey-2022"
        And postgresql cluster with "10.1" minor version and name "postgresql_fast_update_tls_certs"
        And set tls expirations of hosts of cluster "cid8" to "2000-01-01T00:00:00Z"
        And move cluster "cid8" to "dev" env
        And postgresql cluster with "10.1" minor version and name "postgresql_fast_update_minor_version"
        And move cluster "cid9" to "dev" env
        And postgresql cluster with "10.1" minor version and name "postgresql_fast_update_odyssey_version"
        And move cluster "cid10" to "dev" env
        And I set "cid10" "odyssey" version to "0.322"

        And stopped postgresql cluster with name "postgresql_switch_billing"
        And In cluster "cid11" I set pillar path "{data,billing}"
        And In cluster "cid11" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"

        And stopped postgresql cluster with name "postgresql_offline_hs"

    Scenario: Postgresql clusters revisions doesn't change
        * I check that "postgresql" cluster revision doesn't change while pillar_change

    Scenario: Maintain all postgresql configs
        * I maintain all "postgresql" configs
