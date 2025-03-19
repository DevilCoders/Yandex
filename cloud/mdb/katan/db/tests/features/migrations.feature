Feature: Migrations produce same DDL as katan.sql

  @migration
  Scenario: Create database and compare ddl
    Given "katandb_single" database from katan.sql
    Given "katandb_migrated" database from migrations/
    When I dump "katandb_single" to "katandb_single.ddl"
    And I dump "katandb_migrated" to "katandb_migrated.ddl"
    Then there are no differences in "katandb_single.ddl" and "katandb_migrated.ddl"
