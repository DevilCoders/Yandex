Feature: Greenplum maintenance tasks
    Background: Greenplum cluster and MW task
        Given I add default feature flag "MDB_GREENPLUM_CLUSTER"
        And greenplum cluster with name "greenplum_update_tls_certs"
        And worker task "worker_task_id1" is acquired and completed by worker
        And move cluster "cid1" to "dev" env
        And greenplum cluster with name "greenplum_cluster_start_segment_failover"
        And worker task "worker_task_id2" is acquired and completed by worker
        And CMS up and running with task for cluster "greenplum_cluster_start_segment_failover"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """

    Scenario: Greenplum_update_tls_certs config works
        When set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        And I successfully load config with name "greenplum_update_tls_certs"
        Then cluster selection successfully returns
        """
        [cid1]
        """

   Scenario: Greenplum_update_odyssey_version config works
        When I successfully load config with name "greenplum_update_odyssey_version"
        When I set "cid1" "odyssey" version to "0.123"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: Greenplum_update_minor_version config works
        When I set "cid1" cluster "greenplum" major version to "6.17", minor version to "foo"
        When I successfully load config with name "greenplum_update_minor_version"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: Greenplum_update_major_version config works
        When I set "cid1" cluster "greenplum" major version to "6.17", minor version to "foo"
        When I successfully load config with name "greenplum_update_major_version"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """
        And cluster "cid1" has "1" major version

   Scenario: Greenplum_update_minor_version config does not works if we set disable flag
        When I set "cid1" cluster "greenplum" major version to "6.17", minor version to "foo"
        And I successfully load config with name "greenplum_update_minor_version"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When In cluster "cid1" I set pillar path "{data,disable_mw}"
        And In cluster "cid1" I set pillar path "{data,disable_mw,greenplum}" to bool "true"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: Greenplum_update_pxf_version config works
        When I successfully load config with name "greenplum_update_pxf_version"
        When I set "cid1" "pxf" version to "0.123"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: Greenplum_update_pxf_version config does not works if we set disable flag
        When I successfully load config with name "greenplum_update_pxf_version"
        When I set "cid1" "pxf" version to "0.123"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: Greenplum mw task args contains target_maint_vm id
        Given "greenplum_cluster_start_segment_failover" config
        """
        info: Cluster segment host failover
        repeatable: true
        priority: 2
        env_order_disabled: true
        clusters_selection:
            cms:
            - steps:
                endswith: check if primary
              duration: 30m
              cluster_type: greenplum_cluster
        pillar_change: 'SELECT 1
        
        '
        worker:
            operation_type: greenplum_cluster_modify
            task_type: greenplum_cluster_start_segment_failover
            timeout: 10m
            task_args: {}
        max_delay_days: 4
        min_days: 1
        """
        When I maintain "greenplum_cluster_start_segment_failover" config
        Then there is one maintenance task
        And exists "greenplum_cluster_start_segment_failover" maintenance task on "cid2" in "PLANNED" status
        And cluster "cid2" have "greenplum_cluster_start_segment_failover" task with argument "target_maint_vtype_id" equal to
        """
        "test-instance-id"
        """

