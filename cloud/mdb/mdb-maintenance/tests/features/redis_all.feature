Feature: Run all redis maintenance tasks
    Background: Create redis clusters for every config
        Given I add default feature flag "MDB_REDIS_62"
        And I load all configs
        And redis cluster with "6.2.1" minor version and name "redis_update_minor_version"

        And I increase cloud quota
        And redis cluster with "6.2.1" minor version and name "redis_cluster_start_failover"
        And CMS up and running with task for cluster "redis_cluster_start_failover"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """

        And I increase cloud quota
        And redis cluster with "6.2.1" minor version and name "redis_update_tls_certs"
        And In cluster "cid3" I set pillar path "{data,redis,tls,enabled}" to bool "true"
        And set tls expirations of hosts of cluster "cid3" to "2000-01-01T00:00:00Z"

        And I increase cloud quota
        And redis cluster with "6.2.1" minor version and name "redis_switch_billing"
        And In cluster "cid4" I set pillar path "{data,billing}"
        And In cluster "cid4" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"

        And I increase cloud quota
        And I add default feature flag "MDB_REDIS_5"
        And redis cluster with "5.0.1" minor version and name "redis_update_major_5_0"

        And I increase cloud quota
        And I add default feature flag "MDB_REDIS_6"
        And redis cluster with "6.0.1" minor version and name "redis_update_major_6_0"

    Scenario: Redis clusters revisions doesn't change
        * I check that "redis" cluster revision doesn't change while pillar_change

    Scenario: Maintain all redis configs
        * I maintain all "redis" configs
