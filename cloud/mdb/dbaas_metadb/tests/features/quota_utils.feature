Feature: Our different utils

  Scenario: Quota from flavor util
    Given default database
    When I execute query
    """
    SELECT *
      FROM code.flavor_as_quota('flavor1')
    """
    Then it returns one row matches
    """
    cpu: 1.0
    memory: 1
    ssd_space: NULL
    hdd_space: NULL
    clusters: NULL
    """
    When I execute query
    """
    SELECT *
      FROM code.flavor_as_quota(
          'flavor1',
          i_ssd_space=>10,
          i_hdd_space=>20)
    """
    Then it returns one row matches
    """
    cpu: 1.0
    ssd_space: 10
    hdd_space: 20
    """
    When I execute query
    """
       SELECT mq.*
         FROM code.flavor_as_quota('flavor1') oq,
      LATERAL code.multiply_quota(oq, -3) mq
    """
    Then it returns one row matches
    """
    cpu: -3.0
    memory: -3
    ssd_space: NULL
    hdd_space: NULL
    clusters: NULL
    """

  Scenario: Quota from flavor for burstable flavor
    Given default database
    When I execute query
    """
    SELECT *
      FROM code.flavor_as_quota('flavor0.5')
    """
    Then it returns one row matches
    """
    cpu: 0.5
    memory: 1
    ssd_space: NULL
    hdd_space: NULL
    clusters: NULL
    """
