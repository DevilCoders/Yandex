Feature: combine_dict tests
  Background: Just DB
    Given default database

  Scenario: Happy path
    When I execute query
    """
    SELECT code.combine_dict(
        cast(ARRAY[
            '{"users": {"admin": {"pwd": "pwd"}}}',
            '{"users": {"usr": {"pwd": "pwd"}}, "version": 42}'
       ] AS jsonb[])
    ) AS res
    """
    Then it returns one row matches
    """
    res:
      users:
        admin:
          pwd: pwd
        usr:
          pwd: pwd
      version: 42
    """

  Scenario: Override string with mapping
    When I execute query
    """
    SELECT code.combine_dict(
        cast(ARRAY[
            '{"data": {"storage": "s3://xxx"}}',
            '{"data": {"storage": {"scheme": "s3", "path": "xxx"}}}'
       ] AS jsonb[])
    ) AS res
    """
    Then it returns one row matches
    """
    res:
      data:
        storage:
          scheme: s3
          path: xxx
    """

  @MDBSUPPORT-5289
  Scenario: Do not join arrays
    When I execute query
    """
    SELECT code.combine_dict(
        cast(ARRAY[
            '{"data": {"mysql": {"config": {"sql_mode": ["NO_ENGINE_SUBSTITUTION", "NO_ZERO_DATE"]}}}}',
            '{"data": {"mysql": {"config": {"sql_mode": ["NO_ENGINE_SUBSTITUTION"]}}}}'
       ] AS jsonb[])
    ) AS res
    """
    Then it returns one row matches
    """
    res:
      data:
        mysql:
          config:
            sql_mode:
              - NO_ENGINE_SUBSTITUTION
    """
