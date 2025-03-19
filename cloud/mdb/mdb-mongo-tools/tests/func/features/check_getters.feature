Feature: MongoDB resetup: 1P + 1S + 1 Stale

  Background: Wait for working infrastructure
    Given a mongodb hosts mongodb01 mongodb02
      And mongodb is working on all mongodb hosts
      And replset initialized on mongodb01
      And auth initialized on mongodb01
      And mongodb01 role is one of PRIMARY


  Scenario: Tools works fine with non HA replica set
    Then we run getter on mongodb01 with args "is_alive" and exit code is 0
    """
    mongodb is alive
    """
    And we run getter on mongodb01 with args "is_alive --quiet" and exit code is 0
    """
    """
    And we run getter on mongodb01 with args "is_ha" and exit code is 10
    """
    Not enough votes in replset config: 2 < 3
    """
    And we run getter on mongodb01 with args "shutdown_allowed" and exit code is 10
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is not allowed
    """


  Scenario: Test tools with HA replica set
    Given mongodb03 added to replset
    And a mongodb hosts mongodb01 mongodb02 mongodb03
    And all mongodb hosts are synchronized
    Then we run getter on mongodb01 with args "is_ha" and exit code is 0
    """
    Replica set config has more than 3 votes
    """
    And we run getter on mongodb01 with args "shutdown_allowed" and exit code is 0
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is allowed
    """


  Scenario: Test tools with HA replica set without primary
    Given mongodb03 added to replset
    And mongodb is working on all mongodb hosts
    And replset initialized on mongodb01
    And all mongodb hosts are synchronized
    Then we run getter on mongodb01 with args "is_primary" and exit code is 0
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} is primary
    """
    And we run getter on mongodb02 with args "is_primary" and exit code is 10
    """
    mongodb02.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} is not primary
    """
    And we run getter on mongodb01 with args "primary_exists" and exit code is 0
    """
    One primary exists in replica set
    """
    And we run getter on mongodb02 with args "primary_exists" and exit code is 0
    """
    One primary exists in replica set
    """
    And we run getter on mongodb01 with args "is_ha" and exit code is 0
    """
    Replica set config has more than 3 votes
    """
    And we run getter on mongodb01 with args "shutdown_allowed" and exit code is 0
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is allowed
    """


    When we stop mongodb on mongodb02
    And we stop mongodb on mongodb03
    Then we run getter on mongodb01 with args "is_alive" and exit code is 0
    """
    mongodb is alive
    """
    Then we run getter on mongodb02 with args "is_alive" and exit code is 10
    """
    Could not connect to mongodb
    """

    Then we run getter on mongodb01 with args "is_primary" and exit code is 50
    """
    Primary was not found
    """
    And we run getter on mongodb02 with args "is_primary" and exit code is 50
    """
    Could not connect to mongodb
    """

    And we run getter on mongodb01 with args "primary_exists" and exit code is 10
    """
    Primary was not found
    """
    And we run getter on mongodb02 with args "primary_exists" and exit code is 50
    """
    Could not connect to mongodb
    """

    And we run getter on mongodb01 with args "is_ha" and exit code is 0
    """
    Replica set config has more than 3 votes
    """
    And we run getter on mongodb01 with args "shutdown_allowed" and exit code is 10
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is not allowed
    """


    When we start mongodb on mongodb02
    Then we run getter on mongodb01 with args "is_alive --try-number=30" and exit code is 0
    """
    mongodb is alive
    """
    Then we run getter on mongodb01 with args "is_alive" and exit code is 0
    """
    mongodb is alive
    """

    Then we run getter on mongodb01 with args "is_primary --try-number=30" and exit code is 0
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} is primary
    """
    And we run getter on mongodb02 with args "is_primary" and exit code is 10
    """
    mongodb02.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} is not primary
    """

    And we run getter on mongodb01 with args "primary_exists" and exit code is 0
    """
    One primary exists in replica set
    """
    And we run getter on mongodb02 with args "primary_exists" and exit code is 0
    """
    One primary exists in replica set
    """

    And we run getter on mongodb01 with args "shutdown_allowed" and exit code is 10
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is not allowed
    """
    And we run getter on mongodb02 with args "shutdown_allowed" and exit code is 10
    """
    mongodb02.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is not allowed
    """


    When we start mongodb on mongodb03
    Then we run getter on mongodb03 with args "is_alive --try-time=30" and exit code is 0
    """
    mongodb is alive
    """

    Then we run getter on mongodb01 with args "is_primary" and exit code is 0
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} is primary
    """
    And we run getter on mongodb02 with args "is_primary" and exit code is 10
    """
    mongodb02.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} is not primary
    """

    And we run getter on mongodb01 with args "shutdown_allowed" and exit code is 0
    """
    mongodb01.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is allowed
    """
    And we run getter on mongodb02 with args "shutdown_allowed" and exit code is 0
    """
    mongodb02.{{conf.network_name}}:{{conf.projects.mongodb.expose.mongod}} shutdown is allowed
    """
