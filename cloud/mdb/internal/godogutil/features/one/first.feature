@ok
Feature: First features

  Scenario: First scenario in first feature
    Given I running
    Then I should be executed at "1"

  Scenario Outline: Second scenario in first feature
    Given I running
    Then I should be executed at "<num>"

    Examples:
      | num |
      | 2   |
      | 3   |

