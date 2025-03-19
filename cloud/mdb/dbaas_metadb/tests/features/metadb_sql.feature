Feature: Migrations produce same DDL as metadb.sql

    Scenario: Create database and compare ddl
        Given default database
        And database from metadb.sql
        When I dump database from metadb.sql
        And I dump default database
        Then there is no difference between default database and database from metadb.sql

    Scenario: Generate migration with code/ changes
        Given default database
        And database from metadb.sql
        Then I generate new code migration
