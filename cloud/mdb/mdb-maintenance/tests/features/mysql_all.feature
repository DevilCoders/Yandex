Feature: Run all mysql maintenance tasks
    Background: Create mysql clusters for every config
        Given I load all configs
        And mysql cluster with "5.7.25" minor version and name "mysql_5_7_update_tls_certs"
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        And I increase cloud quota
        And mysql cluster with "8.0.17" minor version and name "mysql_8_0_update_tls_certs"
        And set tls expirations of hosts of cluster "cid2" to "2000-01-01T00:00:00Z"
        And mysql cluster with "8.0.1" minor version and name "mysql_update_minor_version"
        And mysql cluster with "8.0.17" minor version and name "mysql_cluster_start_failover"
        And CMS up and running with task for cluster "mysql_cluster_start_failover"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """
        And mysql cluster with "8.0.17" minor version and name "mysql_switch_billing"
        And In cluster "cid5" I set pillar path "{data,billing}"
        And In cluster "cid5" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"

    Scenario: Mysql clusters revisions doesn't change
        * I check that "mysql" cluster revision doesn't change while pillar_change

    Scenario: Maintain all mysql configs
        * I maintain all "mysql" configs
