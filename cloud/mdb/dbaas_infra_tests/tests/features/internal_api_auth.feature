Feature: Internal API Auth

  Background: Wait until internal api is ready
    Given Internal API is up and running
    And actual MetaDB version

  Scenario Outline: Internal API rejects requests not passing authorization
    Given disabled authorization in access service
    When we send request to get a list of <cluster_type> clusters
    Then response should have status 403

    Examples:
      | cluster_type |
      | PostgreSQL   |
      | ClickHouse   |
      | MongoDB      |

  Scenario Outline: Internal API rejects requests not passing authentication
    Given disabled authentication in access service
    When we send request to get a list of <cluster_type> clusters
    Then response should have status 403

    Examples:
      | cluster_type |
      | PostgreSQL   |
      | ClickHouse   |
      | MongoDB      |
