Feature: Lint success

  Scenario: Lint code schema
    Given database at last migration
    And initialized linter
    Then linter find nothing for functions in schema "code"
