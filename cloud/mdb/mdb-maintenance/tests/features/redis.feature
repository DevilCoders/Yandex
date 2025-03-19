Feature: Redis maintenance tasks

    Scenario: Redis minor update works
        Given I add default feature flag "MDB_REDIS_62"
        And redis cluster with "6.2.1" minor version
        When I successfully load config with name "redis_update_minor_version"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: Redis 5.0 major update works
        Given I add default feature flag "MDB_REDIS_62"
        And I add default feature flag "MDB_REDIS_5"
        And redis cluster with "5.0.1" minor version
        When I successfully load config with name "redis_update_major_5_0"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: Redis 6.0 major update works
        Given I add default feature flag "MDB_REDIS_62"
        And I add default feature flag "MDB_REDIS_6"
        And redis cluster with "6.0.1" minor version
        When I successfully load config with name "redis_update_major_6_0"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """
