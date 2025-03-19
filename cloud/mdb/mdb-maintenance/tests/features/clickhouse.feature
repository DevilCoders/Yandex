Feature: ClickHouse maintenance tasks

    Scenario: ClickHouse minor update
        Given clickhouse cluster with "21.3.2.1" version
        When I successfully load config with name "clickhouse_update_minor_21_3"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: ClickHouse minor update doesn't touch other clusters
        Given postgresql cluster
        And I increase cloud quota
        And clickhouse cluster with "21.2.2.1" version and name "clickhouse_21_2"
        And clickhouse cluster with "21.3.99.999" version and name "clickhouse_21_3"
        And clickhouse cluster with "21.4.2.1" version and name "clickhouse_21_4"
        When I successfully load config with name "clickhouse_update_minor_21_3"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: ClickHouse major update
        Given clickhouse cluster with "20.3.3.2" version
          And I successfully load config with name "clickhouse_update_major_20_3"
         When I maintain "clickhouse_update_major_20_3" config
         Then there is one maintenance task
          And exists "clickhouse_update_major_20_3" maintenance task on "cid1" in "PLANNED" status
          And there is last sent single notification to cloud "cloud1" for cluster "cid1"
         When I maintain "clickhouse_update_major_20_3" config
         Then there is one maintenance task


    Scenario: enable_mixed_granularity_parts maintenance works
        Given clickhouse cluster with zookeeper
        When I successfully load config with name "clickhouse_enable_mixed_granularity_parts"
        And I set enable_mixed_granularity_parts on cluster "cid1" to "false"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: enable_mixed_granularity_parts maintenance is not required for new clusters
        Given clickhouse cluster with zookeeper
        When I successfully load config with name "clickhouse_enable_mixed_granularity_parts"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: enable_user_management_v2 maintenance works
        Given clickhouse cluster with zookeeper
        When I successfully load config with name "clickhouse_enable_user_management_v2"
        And I set user_management_v2 on cluster "cid1" to "false"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: enable_user_management_v2 maintenance is not required for new clusters
        Given clickhouse cluster with zookeeper
        When I successfully load config with name "clickhouse_enable_user_management_v2"
        Then cluster selection successfully returns
        """
        []
        """
