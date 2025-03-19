Feature: Migrations produce same DDL as billing.sql

  @migration
  Scenario: Create database and compare ddl
    Given "billingdb_single" database from billing.sql
    Given "billingdb_migrated" database from migrations/
    When I dump "billingdb_single" to "billingdb_single.ddl"
    And I dump "billingdb_migrated" to "billingdb_migrated.ddl"
    Then there are no differences in "billingdb_single.ddl" and "billingdb_migrated.ddl"
