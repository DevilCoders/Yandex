Feature: Migrations produce same DDL as mlock.sql

  Scenario: Create database and compare ddl
    Given "mlockdb_single" database from mlock.sql
    Given "mlockdb_migrated" database from migrations/
    When I dump "mlockdb_single" to "mlockdb_single.ddl"
    And I dump "mlockdb_migrated" to "mlockdb_migrated.ddl"
    Then there are no differences in "mlockdb_single.ddl" and "mlockdb_migrated.ddl"
