Feature: Run all clickHouse maintenance tasks
    Background: Create clickHouse clusters for every config
        Given I load all configs except ones that match "^clickhouse_update_(minor|major)_.+$"
        And clickhouse cluster with zookeeper and name "clickhouse_add_acl_without_tls"
        And In cluster "cid1" I set pillar path "{data,unmanaged,enable_zk_tls}" to bool "false"
        And I increase cloud quota
        And clickhouse cluster with zookeeper and name "clickhouse_add_tls"
        And In cluster "cid2" I set pillar path "{data,unmanaged,enable_zk_tls}" to bool "false"
        And clickhouse cluster with zookeeper and name "clickhouse_enable_mixed_granularity_parts"
        And I set enable_mixed_granularity_parts on cluster "cid3" to "false"
        And clickhouse cluster with "20.7.0.0" version and name "clickhouse_enable_user_management_v2"
        And I set user_management_v2 on cluster "cid4" to "false"
        And clickhouse cluster with "19.14.6.0" version and name "clickhouse_fix_temporary_directories_lifetime"
        And In cluster "cid5" I set pillar path "{data,unmanaged,restart_to_fix_temporary_directories_lifetime}" to bool "true"
        And clickhouse cluster with "19.14.6.0" version and name "clickhouse_update_tls"
        And set tls expirations of hosts of cluster "cid6" to "2000-01-01T00:00:00Z"
        And clickhouse cluster with "21.6.0.0" version and name "clickhouse_update_for_clusters_with_cloud_storage"
        And I set cloud_storage on cluster "cid7" to "true"
        And clickhouse cluster with zookeeper and name "clickhouse_update_zookeeper_3_6_3"
        And I set zookeeper version on cluster "cid8" to "3.5.9-1+yandex25-9120b6b"

        And stopped clickhouse cluster with name "clickhouse_switch_billing"
        And In cluster "cid9" I set pillar path "{data,billing}"
        And In cluster "cid9" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"

    Scenario: Clickhouse clusters revisions doesn't change
        Given I check that "clickhouse" cluster revision doesn't change while pillar_change

    Scenario: Maintain all clickhouse configs
        Given I maintain all "clickhouse" configs
