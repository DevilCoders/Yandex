Feature: Common maintenance tasks
    Background: Load configs
        Given I load all configs
        And I add "postgresql" database
        And I add "clickhouse" database
        And I add "mongodb" database
        And I add "redis" database
        And I add "mysql" database
        And I add "kafka" database
        And I add "greenplum" database
        And I add "elasticsearch" database

    Scenario: Validate config names
        Given I check config names starts with db names
