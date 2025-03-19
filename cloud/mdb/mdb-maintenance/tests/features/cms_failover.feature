Feature: CMS Tasks maintained

    Background: PostgreSQL cluster and MW task
        Given postgresql cluster with name "pgtest1" and "pgbouncer" connection_pooler
        And "cms_postgresql_failover" config
        """
        description: Failover Postgresql
        clusters_selection:
          cms:
            - steps:
                endswith: check if primary
              duration: 30m
              cluster_type: postgresql_cluster
        pillar_change: >
          SELECT 1
        worker:
          operation_type: postgresql_cluster_start_failover
          task_type: postgresql_cluster_maintenance
          timeout: 10m
          task_args: { }
        """
        Given CMS up and running with task for cluster "pgtest1"
        """
        executed_steps:
        - check if primary
        created_from_now: 1h
        instance_id: test-instance-id
        """

    Scenario: Maintain switchover
        When I maintain "cms_postgresql_failover" config
        Then there is one maintenance task
        And exists "cms_postgresql_failover" maintenance task on "cid1" in "PLANNED" status
        And there is last sent single notification to cloud "cloud1" for cluster "cid1"
        When I maintain "cms_postgresql_failover" config
        Then there is one maintenance task
