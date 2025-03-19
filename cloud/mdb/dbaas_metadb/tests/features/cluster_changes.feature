Feature: Cluster changes

    Background: Default database with cloud and folder
        Given default database
        And cloud with default quota
        And folder

    Scenario: Attempt to modify cluster in committed rev
        Given cluster with pillar
        """
        {"data": {"version": "10.2"}}
        """
        When I execute query
        """
        SELECT * FROM code.update_pillar(
            i_cid   => :cid,
            i_rev   => 1,
            i_key   => code.make_pillar_key(i_cid=>:cid),
            i_value => '{"data": {"version": "11g.r2"}}'
        )
        """
        Then it fail with error matches "Attempt to modify cluster in different transaction"
