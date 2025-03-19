Feature: Locks API

  Background:
    Given database at last migration

  Scenario: Releasing non-existing lock fails
    When I execute query
    """
    SELECT code.release_lock('no such lock')
    """
    Then it fail with error "Unable to find lock with id no such lock"

  Scenario: Creating lock without objects fails
    When I execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'bad lock',
      '{}'
    )
    """
    Then it fail with error "i_objects should be not empty"

  Scenario: Single lock is immediately acquired
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'single lock',
      '{"host1","host2"}'
    )
    """
    And I execute query
    """
    SELECT * FROM code.get_lock_status('test1')
    """
    Then it returns one row matches
    """
    acquired: true
    conflicts: null
    holder: func-test
    lock_ext_id: test1
    objects:
      - host1
      - host2
    reason: single lock
    """

  Scenario: Retrying lock creation works
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    Then I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """

  Scenario: Retrying lock creation with non-matching objects order works
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    Then I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host2","host1"}'
    )
    """

  Scenario: Creating duplicate lock with non-matching holder fails
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    And I execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test2',
      'first lock',
      '{"host1","host2"}'
    )
    """
    Then it fail with error "Found existing lock test1 with holder func-test"

  Scenario: Creating duplicate lock with non-matching reason fails
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    And I execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'second lock',
      '{"host1","host2"}'
    )
    """
    Then it fail with error "Found existing lock test1 with reason first lock"

  Scenario: Creating duplicate lock with non-matching objects fails
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    And I execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host3","host2"}'
    )
    """
    Then it fail with error "Found existing lock test1 with objects host1, host2"

  Scenario: Lock with conflicts is acquired after conflicting lock is released
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    And I successfully execute query
    """
    SELECT code.create_lock(
      'test2',
      'func-test',
      'second lock',
      '{"host1","host2"}'
    )
    """
    And I execute query
    """
    SELECT * FROM code.get_lock_status('test2')
    """
    Then it returns one row matches
    """
    acquired: false
    conflicts:
      - ids:
          - test1
        object: host1
      - ids:
          - test1
        object: host2
    holder: func-test
    lock_ext_id: test2
    objects:
      - host1
      - host2
    reason: second lock
    """
    When I successfully execute query
    """
    SELECT code.release_lock('test1')
    """
    And I execute query
    """
    SELECT * FROM code.get_lock_status('test2')
    """
    Then it returns one row matches
    """
    acquired: true
    conflicts: null
    holder: func-test
    lock_ext_id: test2
    objects:
      - host1
      - host2
    reason: second lock
    """

  Scenario: Lock listing works
    When I successfully execute query
    """
    SELECT code.create_lock(
      'test1',
      'func-test',
      'first lock',
      '{"host1","host2"}'
    )
    """
    And I successfully execute query
    """
    SELECT code.create_lock(
      'test2',
      'func-test',
      'second lock',
      '{"host1","host2"}'
    )
    """
    And I execute query
    """
    SELECT * FROM code.get_locks()
    """
    Then it returns "2" rows matches
    """
    - holder: func-test
      lock_ext_id: test1
      objects:
        - host1
        - host2
      reason: first lock
    - holder: func-test
      lock_ext_id: test2
      objects:
        - host1
        - host2
      reason: second lock
    """
