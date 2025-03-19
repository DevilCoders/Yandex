Feature: MongoDB maintenance tasks

    Scenario: MongoDB minor update works
        Given mongodb cluster with "4.0" major version
        When I successfully load config with name "mongodb_update_minor_4_0_27"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: MongoDB minor update works with filled full_num
        Given mongodb cluster with "40022" full_num version
        When I successfully load config with name "mongodb_update_minor_4_0_27"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: MongoDB minor update will not downgrade
        Given mongodb cluster with "40099" full_num version
        When I successfully load config with name "mongodb_update_minor_4_0_27"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: MongoDB minor update doesn't touch other clusters
        Given postgresql cluster
        And mongodb cluster with "4.2" major version
        When I successfully load config with name "mongodb_update_minor_4_0_27"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: MongoDB major 3.6 update works
        Given mongodb cluster with "3.6" major version
        And move cluster "cid1" to "qa" env
        When I successfully load config with name "mongodb_update_major_3_6_qa"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

