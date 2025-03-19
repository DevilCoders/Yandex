Feature: Run all mongodb maintenance tasks
    Background: Create mongodb clusters for every config
        Given I load all configs
        And mongodb cluster with "30600" full_num version and name "mongodb_update_major_3_6_prod"
        And In cluster "cid1" I set pillar path "{data,update_4_0}" to bool "true"
        And move cluster "cid1" to "prod" env
        And I increase cloud quota
        And mongodb cluster with "40409" full_num version and name "mongodb_update_tls"
        And set tls expirations of hosts of cluster "cid2" to "2000-01-01T00:00:00Z"
        And mongodb cluster with "30600" full_num version and name "mongodb_update_major_3_6_qa"
        And mongodb cluster with "50000" full_num version and name "mongodb_update_minor_version"
	      And I set "cid4" cluster "mongodb" major version to "5.0", minor version to "0"
        And mongodb cluster with "30600" full_num version and name "mongodb_switch_billing"
        And In cluster "cid5" I set pillar path "{data,billing}"
        And In cluster "cid5" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"
        And mongodb cluster with "40000" full_num version and name "mongodb_update_fcv_4_0"
        And In cluster "cid6" I set pillar path "{data,mongodb,feature_compatibility_version}" to string "3.6"
        And mongodb cluster with "40000" full_num version and name "mongodb_update_major_4_0"
	      And I set "cid7" cluster "mongodb" major version to "4.0", minor version to "0"

        And mongodb cluster with "50000" full_num version and name "mongodb_cluster_start_failover"
        And CMS up and running with task for cluster "mongodb_cluster_start_failover"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """

    Scenario: Mongodb clusters revisions doesn't change
        Given I check that "mongodb" cluster revision doesn't change while pillar_change

    Scenario: Maintain all mongodb configs
        Given I maintain all "mongodb" configs
