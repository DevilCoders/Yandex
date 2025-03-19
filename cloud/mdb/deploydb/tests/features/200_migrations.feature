Feature: Migrations produce same DDL as deploy.sql

  Scenario: Create database and compare ddl
    Given "deploydb_single" database from deploy.sql
    Given "deploydb_migrated" database from migrations/
    When I dump "deploydb_single" to "deploydb_single.ddl"
    And I dump "deploydb_migrated" to "deploydb_migrated.ddl"
    Then there are no differences in "deploydb_single.ddl" and "deploydb_migrated.ddl"
