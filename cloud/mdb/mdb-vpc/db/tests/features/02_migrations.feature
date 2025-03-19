Feature: Migrations produce same DDL as vpc.sql

  Scenario: Create database and compare ddl
    Given "vpcdb_single" database from vpcdb.sql
    Given "vpcdb_migrated" database from migrations/
    When I dump "vpcdb_single" to "vpcdb_single.ddl"
    And I dump "vpcdb_migrated" to "vpcdb_migrated.ddl"
    Then there are no differences in "vpcdb_single.ddl" and "vpcdb_migrated.ddl"
