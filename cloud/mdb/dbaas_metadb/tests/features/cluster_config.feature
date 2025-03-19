Feature: Managed Cluster Config
  Background: Default database
    Given default database

  Scenario Outline: IO Limits calculation
    When I execute query
    """
    SELECT code.dynamic_io_limit(
        <space_limit>,
        CAST(ROW(1, 'disk', 'ssd'::dbaas.space_quota_type, NULL, <allocation_unit_size>, <io_limit_per_allocation_unit>, <io_limit_max>) AS dbaas.disk_type),
        <static_io_limit>) AS io_limit
    """
    Then it returns one row matches
    """
    io_limit: <expected>
    """

  Examples:
    |  space_limit | allocation_unit_size | io_limit_per_allocation_unit | io_limit_max | static_io_limit | expected |
    |         NULL |                 NULL |                         NULL |         NULL |            1000 |     1000 |
    |       500000 |                 NULL |                         NULL |         NULL |            1000 |     1000 |
    | 214748364800 |           1073741824 |                            1 |         1000 |            1000 |      200 |
    | 214748364800 |                    1 |                         1000 |          512 |            1000 |      512 |
    | 214748364800 |                    1 |                            1 |          512 |            1000 |      512 |


