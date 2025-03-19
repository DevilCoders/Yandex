Feature: Migrations produce same DDL as cms.sql

  Scenario: Create database and compare ddl
    Given "cms_single" database from cms.sql
    Given "cms_migrated" database from migrations/
    When I dump "cms_single" to "cms_single.ddl"
    And I dump "cms_migrated" to "cms_migrated.ddl"
    Then there are no differences in "cms_single.ddl" and "cms_migrated.ddl"
