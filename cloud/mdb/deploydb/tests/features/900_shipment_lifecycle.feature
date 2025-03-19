Feature: Shipment lifecycle

    Scenario: Create shipment with 3 commands on 4 minions
      Given database at last migration
      And "testing" group
      And open alive "master" master in "testing" group
      And "minion1" minion in "testing" group
      And "minion2" minion in "testing" group
      And "minion3" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(ARRAY[ ('pre.hs', '{}', NULL),
                                               ('hs', '{}', NULL) ] AS code.command_def[]),
          i_fqdns               => '{"minion1", "minion2", "minion3"}',
          i_parallel            => 2,
          i_stop_on_error_count => 3,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it returns one row matches
      """
      shipment_id: 1
      status: INPROGRESS
      parallel: 2
      stop_on_error_count: 3
      other_count: 3
      done_count: 0
      errors_count: 0
      total_count: 3
      """
      When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42)
      """
      Then it returns "2" rows matches
      """
      - fqdn: minion1
        type: pre.hs
        id: 1
      - fqdn: minion2
        type: pre.hs
        id: 2
      """
      When I successfully execute query
      """
      SELECT * FROM code.create_job('jid-1', 1)
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job_result('jid-1', 'minion1', 'SUCCESS', '{}')
      """
      And I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: INPROGRESS
      other_count: 3
      done_count: 0
      errors_count: 0
      total_count: 3
      """
      When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42)
      """
      Then it returns "2" rows matches
      """
      - fqdn: minion2
        type: pre.hs
        id: 2
      - fqdn: minion1
        type: hs
        id: 4
      """
      When I successfully execute query
      """
      SELECT code.create_job('jid-2', id)
        FROM code.get_commands_for_dispatch('master', 42)
       WHERE fqdn = 'minion1'
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job_result('jid-2', 'minion1', 'FAILURE', '{}')
      """
      When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42)
      """
      Then it returns "2" rows matches
      """
      - fqdn: minion2
        type: pre.hs
        id: 2
      - fqdn: minion3
        type: pre.hs
        id: 3
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: INPROGRESS
      other_count: 2
      done_count: 0
      errors_count: 1
      total_count: 3
      """
      When I successfully execute query
      """
      SELECT code.create_job('jid-3', id)
        FROM code.get_commands_for_dispatch('master', 42)
       WHERE fqdn = 'minion2'
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job_result('jid-3', 'minion2', 'FAILURE', '{}')
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: INPROGRESS
      other_count: 1
      done_count: 0
      errors_count: 2
      total_count: 3
      """
      When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42)
      """
      Then it returns one row matches
      """
      fqdn: minion3
      type: pre.hs
      id: 3
      """
      When I successfully execute query
      """
      SELECT code.create_job('jid-4', id)
        FROM code.get_commands_for_dispatch('master', 42)
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job_result('jid-4', 'minion3', 'SUCCESS', '{}')
      """
      When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42)
      """
      Then it returns one row matches
      """
      fqdn: minion3
      type: hs
      id: 6
      """
      When I successfully execute query
      """
      SELECT code.create_job('jid-5', id)
        FROM code.get_commands_for_dispatch('master', 42)
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job_result('jid-5', 'minion3', 'SUCCESS', '{}')
      """
      When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42)
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: DONE
      other_count: 0
      done_count: 1
      errors_count: 2
      total_count: 3
      """

  Scenario: Create shipment and timeout it
    Given database at last migration
    And "testing" group
    And open alive "master" master in "testing" group
    And "minion" minion in "testing" group
    And "minion2" minion in "testing" group
    And "minion3" minion in "testing" group
    And "minion4" minion in "testing" group
    And "minion5" minion in "testing" group
    And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(ARRAY[ ('pre.hs', '{}', NULL),
                                               ('hs', '{}', NULL) ] AS code.command_def[]),
          i_fqdns               => '{"minion"}',
          i_parallel            => 1,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 minute')
      """
    And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(ARRAY[ ('hs2', '{}', NULL) ] AS code.command_def[]),
          i_fqdns               => '{"minion", "minion2", "minion3", "minion4", "minion5"}',
          i_parallel            => 4,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 minute')
      """
    And successfully executed query
      """
      SELECT code.create_job('jid4', 4)
      """
    And successfully executed query
      """
      SELECT code.create_job('jid5', 5)
      """
    And successfully executed query
      """
      SELECT code.create_job_result('jid5', 'minion3', 'SUCCESS', '{}')
      """
    And successfully executed query
      """
      SELECT code.create_job('jid6', 6)
      """
    And successfully executed query
      """
      SELECT code.create_job_result('jid6', 'minion4', 'FAILURE', '{}')
      """
    And successfully executed query
      """
      SELECT code.create_job('jid7', 7)
      """
    And successfully executed query
      """
      SELECT code.create_job_result('jid7', 'minion5', 'TIMEOUT', '{}')
      """
    When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - shipment_id: 1
        status: INPROGRESS
        other_count: 1
        done_count: 0
        errors_count: 0
        total_count: 1
      - shipment_id: 2
        status: INPROGRESS
        other_count: 2
        done_count: 1
        errors_count: 2
        total_count: 5
      """
    When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
    Then it returns "7" rows matches
      """
      - id: 1
        shipment_id: 1
        fqdn: minion
        status: AVAILABLE
      - id: 2
        shipment_id: 1
        fqdn: minion
        status: BLOCKED
      - id: 3
        shipment_id: 2
        fqdn: minion
        status: AVAILABLE
      - id: 4
        shipment_id: 2
        fqdn: minion2
        status: RUNNING
      - id: 5
        shipment_id: 2
        fqdn: minion3
        status: DONE
      - id: 6
        shipment_id: 2
        fqdn: minion4
        status: ERROR
      - id: 7
        shipment_id: 2
        fqdn: minion5
        status: TIMEOUT
      """
    When I execute query
      """
      SELECT * FROM code.timeout_shipments(100)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - shipment_id: 1
        status: INPROGRESS
        other_count: 1
        done_count: 0
        errors_count: 0
        total_count: 1
      - shipment_id: 2
        status: INPROGRESS
        other_count: 2
        done_count: 1
        errors_count: 2
        total_count: 5
      """
    When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
    Then it returns "7" rows matches
      """
      - id: 1
        shipment_id: 1
        fqdn: minion
        status: AVAILABLE
      - id: 2
        shipment_id: 1
        fqdn: minion
        status: BLOCKED
      - id: 3
        shipment_id: 2
        fqdn: minion
        status: AVAILABLE
      - id: 4
        shipment_id: 2
        fqdn: minion2
        status: RUNNING
      - id: 5
        shipment_id: 2
        fqdn: minion3
        status: DONE
      - id: 6
        shipment_id: 2
        fqdn: minion4
        status: ERROR
      - id: 7
        shipment_id: 2
        fqdn: minion5
        status: TIMEOUT
      """
    Given successfully executed query
      """
      UPDATE deploy.shipments SET created_at = created_at - INTERVAL '1 minute'
      """
    When I execute query
      """
      SELECT * FROM code.timeout_shipments(1)
      """
    Then it returns "1" rows matches
      """
      - shipment_id: 1
        status: TIMEOUT
        other_count: 1
        done_count: 0
        errors_count: 0
        total_count: 1
      """
    When I execute query
      """
      SELECT * FROM code.timeout_shipments(1)
      """
    Then it returns "1" rows matches
      """
      - shipment_id: 2
        status: TIMEOUT
        other_count: 2
        done_count: 1
        errors_count: 2
        total_count: 5
      """
    When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
    Then it returns "7" rows matches
      """
      - id: 1
        shipment_id: 1
        fqdn: minion
        status: CANCELED
      - id: 2
        shipment_id: 1
        fqdn: minion
        status: CANCELED
      - id: 3
        shipment_id: 2
        fqdn: minion
        status: CANCELED
      - id: 4
        shipment_id: 2
        fqdn: minion2
        status: RUNNING
      - id: 5
        shipment_id: 2
        fqdn: minion3
        status: DONE
      - id: 6
        shipment_id: 2
        fqdn: minion4
        status: ERROR
      - id: 7
        shipment_id: 2
        fqdn: minion5
        status: TIMEOUT
      """
    When I execute query
      """
      SELECT * FROM code.timeout_shipments(100)
      """
    Then it returns nothing

  Scenario: Create shipment and throttle it
    Given database at last migration
    And "testing" group
    And open alive "master" master in "testing" group
    And "minion" minion in "testing" group
    And "minion2" minion in "testing" group
    And "minion3" minion in "testing" group
    And "minion4" minion in "testing" group
    And "minion5" minion in "testing" group
    And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(ARRAY[ ('pre.hs', '{}', NULL),
                                               ('hs', '{}', NULL) ] AS code.command_def[]),
          i_fqdns               => '{"minion"}',
          i_parallel            => 1,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 minute')
      """
    And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(ARRAY[ ('hs2', '{}', NULL) ] AS code.command_def[]),
          i_fqdns               => '{"minion", "minion2", "minion3", "minion4", "minion5"}',
          i_parallel            => 4,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 minute')
      """
    And successfully executed query
      """
      SELECT code.create_job('jid3', 3)
      """
    And successfully executed query
      """
      SELECT code.create_job_result('jid3', 'minion', 'SUCCESS', '{}')
      """
    And successfully executed query
      """
      SELECT code.create_job('jid4', 4)
      """
    And successfully executed query
      """
      SELECT code.create_job('jid5', 5)
      """
    And successfully executed query
      """
      SELECT code.create_job_result('jid5', 'minion3', 'SUCCESS', '{}')
      """
    And successfully executed query
      """
      SELECT code.create_job('jid6', 6)
      """
    And successfully executed query
      """
      SELECT code.create_job('jid7', 7)
      """
    And successfully executed query
      """
      SELECT code.create_job_result('jid7', 'minion5', 'TIMEOUT', '{}')
      """
    When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - shipment_id: 1
        status: INPROGRESS
        other_count: 1
        done_count: 0
        errors_count: 0
        total_count: 1
      - shipment_id: 2
        status: INPROGRESS
        other_count: 2
        done_count: 2
        errors_count: 1
        total_count: 5
      """
    When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
    Then it returns "7" rows matches
      """
      - id: 1
        shipment_id: 1
        fqdn: minion
        status: AVAILABLE
      - id: 2
        shipment_id: 1
        fqdn: minion
        status: BLOCKED
      - id: 3
        shipment_id: 2
        fqdn: minion
        status: DONE
      - id: 4
        shipment_id: 2
        fqdn: minion2
        status: RUNNING
      - id: 5
        shipment_id: 2
        fqdn: minion3
        status: DONE
      - id: 6
        shipment_id: 2
        fqdn: minion4
        status: RUNNING
      - id: 7
        shipment_id: 2
        fqdn: minion5
        status: TIMEOUT
      """
    When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42, 2)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42, 1)
      """
    Then it returns nothing
    When successfully executed query
      """
      SELECT code.create_job_result('jid6', 'minion4', 'FAILURE', '{}')
      """
    When I execute query
      """
      SELECT id, fqdn, type
        FROM code.get_commands_for_dispatch('master', 42, 2)
      """
    Then it returns one row matches
      """
      id: 1
      fqdn: minion
      type: pre.hs
      """
