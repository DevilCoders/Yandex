Feature: Groups API

    Scenario: Create group and get it
       Given database at last migration
        When I execute query
        """
        SELECT * FROM code.create_group('prod')
        """
        Then it returns one row matches
        """
        name: prod
        """
        When I execute query
        """
        SELECT * FROM code.get_group('prod')
        """
        Then it returns one row matches
        """
        name: prod
        """

    Scenario: Create 2 groups
       Given database at last migration
        And successfully executed query
        """
        SELECT code.create_group('prod') AS name
        """
        And successfully executed query
        """
        SELECT code.create_group('testing') AS name
        """
        When I execute query
        """
        SELECT * FROM code.get_groups(
          i_limit          => 100,
          i_last_group_id  => NULL
        )
        """
        Then it returns "2" rows matches
        """
        - name: prod
        - name: testing
        """
        When I execute query
           """
           SELECT * FROM code.get_groups(
             i_limit          => 1,
             i_last_group_id  => NULL
           )
           """
        Then it returns "1" rows matches
           """
           - name: prod
           """
        When I execute query
           """
           SELECT * FROM code.get_groups(
             i_limit          => 1,
             i_last_group_id  => 1
           )
           """
        Then it returns "1" rows matches
           """
           - name: testing
           """
        When I execute query
              """
              SELECT * FROM code.get_groups(
                i_limit          => 100,
                i_last_group_id  => 2
              )
              """
        Then it returns nothing
