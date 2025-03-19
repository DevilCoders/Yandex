Feature: Run all greenplum maintenance tasks
    Background: Create greenplum clusters for every config
        Given I load all configs
        And I add default feature flag "MDB_GREENPLUM_CLUSTER"
        And greenplum cluster with name "greenplum_update_tls_certs"
        And worker task "worker_task_id1" is acquired and completed by worker
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        And move cluster "cid1" to "dev" env
        And I increase cloud quota
        And greenplum cluster with name "greenplum_update_odyssey_version"
        And worker task "worker_task_id2" is acquired and completed by worker
        And I set "cid2" "odyssey" version to "0.123"
        And move cluster "cid2" to "dev" env
        And greenplum cluster with name "greenplum_update_minor_version"
        And worker task "worker_task_id3" is acquired and completed by worker
        And move cluster "cid3" to "dev" env
        And I set "cid3" cluster "greenplum" major version to "6.17", minor version to "foo"
        And greenplum cluster with name "greenplum_update_pxf_version"
        And worker task "worker_task_id4" is acquired and completed by worker
        And I set "cid4" "pxf" version to "0.123"
        And move cluster "cid4" to "dev" env
        And greenplum cluster with name "greenplum_update_major_version"
        And worker task "worker_task_id5" is acquired and completed by worker
        And move cluster "cid5" to "dev" env
        And I set "cid5" cluster "greenplum" major version to "6.17", minor version to "foo"
        And greenplum cluster with name "greenplum_switch_billing"
        And worker task "worker_task_id6" is acquired and completed by worker
        And In cluster "cid6" I set pillar path "{data,billing}"
        And In cluster "cid6" I set pillar path "{data,billing,use_cloud_logbroker}" to bool "false"
        And greenplum cluster with name "greenplum_cluster_start_segment_failover"
        And worker task "worker_task_id7" is acquired and completed by worker
        And CMS up and running with task for cluster "greenplum_cluster_start_segment_failover"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """

    Scenario: Greenplum clusters revisions doesn't change
        * I check that "greenplum" cluster revision doesn't change while pillar_change

    Scenario: Maintain all greenplum configs
        * I maintain all "greenplum" configs
