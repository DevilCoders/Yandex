Feature: Disk limit

  Background: Default database
    Given default database

  Scenario Outline: IO Limits calculation by flavor and disk_type
    When I execute query
    """
    SELECT code.get_disk_limit(
        <space_limit>,
        <disk_type>,
        <flavor>
    ) AS io_limit
    """
    Then it returns one row matches
    """
    io_limit: <expected>
    """

    Examples:
      | space_limit   | disk_type    | flavor      | expected  |
      | 4194304       | 'disk_type1' | 'flavor1'   | 1         |
      | 4194304       | 'disk_type1' | 'flavorG4'  | 4096      |
      | 1099511627776 | 'disk_type1' | 'flavorG4'  | 536870912 |
      | 4194304       | 'disk_type1' | 'flavorC1'  | 4096      |

  Scenario: Unknown disk_type
    When I execute query
    """
    SELECT code.get_disk_limit(42, 'qwerty', 'flavor1')
    """
    Then it fail with error matches "Unknown disk_type: qwerty"

  Scenario: Unknown flavor
    When I execute query
    """
    SELECT code.get_disk_limit(42, 'disk_type1', 'qwerty')
    """
    Then it fail with error matches "Unknown resource_preset: qwerty"
