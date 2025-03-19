Feature: Shipments API

    Background: Database with group and alive master
      Given database at last migration
      And "testing" group
      And open alive "master" master in "testing" group

    Scenario: Create shipment and get it
      Given "minion" minion in "testing" group
      And "minion2" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type', '{"test=True"}', '40 minutes') AS code.command_def)],
          i_fqdns               => '{"minion", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 2,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type", "arguments": ["test=True"], "timeout": "00:40:00"}]
      fqdns:
       - minion
       - minion2
      status: INPROGRESS
      parallel: 2
      stop_on_error_count: 2
      other_count: 2
      done_count: 0
      errors_count: 0
      total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type", "arguments": ["test=True"], "timeout": "00:40:00"}]
      fqdns:
       - minion
       - minion2
      status: INPROGRESS
      parallel: 2
      stop_on_error_count: 2
      other_count: 2
      done_count: 0
      errors_count: 0
      total_count: 2
      """

    Scenario: Create 2 shipments
      Given "minion" minion in "testing" group
      And "minion2" minion in "testing" group
      And "minion3" minion in "testing" group
      And "minion4" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type1', '{"a", "b=c"}', '1 minute') AS code.command_def)],
          i_fqdns               => '{"minion", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type2', '{"d=e", "f"}', '5 minutes') AS code.command_def)],
          i_fqdns               => '{"minion3", "minion4"}',
          i_parallel            => 1,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '2 hours')
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
        commands: [{"type": "type1", "arguments": ["a", "b=c"], "timeout": "00:01:00"}]
        fqdns:
         - minion
         - minion2
        status: INPROGRESS
        parallel: 2
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      - shipment_id: 2
        commands: [{"type": "type2", "arguments": ["d=e", "f"], "timeout": "00:05:00"}]
        fqdns:
         - minion3
         - minion4
        status: INPROGRESS
        parallel: 1
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => 'minion',
        i_status            => NULL,
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - shipment_id: 1
        commands: [{"type": "type1", "arguments": ["a", "b=c"], "timeout": "00:01:00"}]
        fqdns:
         - minion
         - minion2
        status: INPROGRESS
        parallel: 2
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => 'wrong',
        i_status            => NULL,
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => 'INPROGRESS',
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - shipment_id: 1
        commands: [{"type": "type1", "arguments": ["a", "b=c"], "timeout": "00:01:00"}]
        fqdns:
         - minion
         - minion2
        status: INPROGRESS
        parallel: 2
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      - shipment_id: 2
        commands: [{"type": "type2", "arguments": ["d=e", "f"], "timeout": "00:05:00"}]
        fqdns:
         - minion3
         - minion4
        status: INPROGRESS
        parallel: 1
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => 'ERROR',
        i_limit             => 100,
        i_last_shipment_id  => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 1,
        i_last_shipment_id  => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - shipment_id: 1
        commands: [{"type": "type1", "arguments": ["a", "b=c"], "timeout": "00:01:00"}]
        fqdns:
         - minion
         - minion2
        status: INPROGRESS
        parallel: 2
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 1,
        i_last_shipment_id  => 1
      )
      """
      Then it returns "1" rows matches
      """
      - shipment_id: 2
        commands: [{"type": "type2", "arguments": ["d=e", "f"], "timeout": "00:05:00"}]
        fqdns:
         - minion3
         - minion4
        status: INPROGRESS
        parallel: 1
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 1,
        i_last_shipment_id  => 2
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 2,
        i_last_shipment_id  => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - shipment_id: 1
        commands: [{"type": "type1", "arguments": ["a", "b=c"], "timeout": "00:01:00"}]
        fqdns:
         - minion
         - minion2
        status: INPROGRESS
        parallel: 2
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      - shipment_id: 2
        commands: [{"type": "type2", "arguments": ["d=e", "f"], "timeout": "00:05:00"}]
        fqdns:
         - minion3
         - minion4
        status: INPROGRESS
        parallel: 1
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 2,
        i_last_shipment_id  => 1
      )
      """
      Then it returns "1" rows matches
      """
      - shipment_id: 2
        commands: [{"type": "type2", "arguments": ["d=e", "f"], "timeout": "00:05:00"}]
        fqdns:
         - minion3
         - minion4
        status: INPROGRESS
        parallel: 1
        stop_on_error_count: 0
        other_count: 2
        done_count: 0
        errors_count: 0
        total_count: 2
      """

    Scenario: Create 2 shipments and list in two way
      Given "minion" minion in "testing" group
      And "minion2" minion in "testing" group
      And "minion3" minion in "testing" group
      And "minion4" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type1', '{"a", "b=c"}', '1 minute') AS code.command_def)],
          i_fqdns               => '{"minion", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type2', '{"d=e", "f"}', '5 minutes') AS code.command_def)],
          i_fqdns               => '{"minion3", "minion4"}',
          i_parallel            => 1,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '2 hours')
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 1,
        i_last_shipment_id  => 1,
        i_ascending         => true
      )
      """
      Then it returns one row matches
      """
      shipment_id: 2
      commands: [{"type": "type2", "arguments": ["d=e", "f"], "timeout": "00:05:00"}]
      fqdns:
       - minion3
       - minion4
      status: INPROGRESS
      parallel: 1
      stop_on_error_count: 0
      other_count: 2
      done_count: 0
      errors_count: 0
      total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipments(
        i_fqdn              => NULL,
        i_status            => NULL,
        i_limit             => 1,
        i_last_shipment_id  => 2,
        i_ascending         => false
      )
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type1", "arguments": ["a", "b=c"], "timeout": "00:01:00"}]
      fqdns:
       - minion
       - minion2
      status: INPROGRESS
      parallel: 2
      stop_on_error_count: 0
      other_count: 2
      done_count: 0
      errors_count: 0
      total_count: 2
      """

    Scenario: Fail one command in shipment
      Given "minion" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type', '{}', NULL) AS code.command_def)],
          i_fqdns               => '{"minion"}',
          i_parallel            => 1,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('11', 1)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job_result('11', 'minion', 'FAILURE', '{}')
      """
      When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
      Then it returns one row matches
      """
      status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.get_command(1)
      """
      Then it returns one row matches
      """
      status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: DONE
      errors_count: 1
      total_count: 1
      """

    Scenario: Fail two commands in shipment
      Given "minion" minion in "testing" group
      And "minion2" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type', '{}', NULL) AS code.command_def)],
          i_fqdns               => '{"minion", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('11', 1)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('12', 2)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job_result('11', 'minion', 'FAILURE', '{}')
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: INPROGRESS
      errors_count: 1
      total_count: 2
      """
      Given successfully executed query
      """
      SELECT * FROM code.create_job_result('12', 'minion2', 'FAILURE', '{}')
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: DONE
      errors_count: 2
      total_count: 2
      """

    Scenario: Fail shipment with 1/4 command success
      Given "minion" minion in "testing" group
      And "minion2" minion in "testing" group
      And "minion3" minion in "testing" group
      And "minion4" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type', '{}', NULL) AS code.command_def)],
          i_fqdns               => '{"minion", "minion2", "minion3", "minion4"}',
          i_parallel            => 3,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
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
      Then it returns "4" rows matches
      """
      - id: 1
        fqdn: minion
        status: AVAILABLE
      - id: 2
        fqdn: minion2
        status: AVAILABLE
      - id: 3
        fqdn: minion3
        status: AVAILABLE
      - id: 4
        fqdn: minion4
        status: BLOCKED
      """
      When I successfully execute query
      """
      SELECT * FROM code.create_job('11', 1)
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job('12', 2)
      """
      And I successfully execute query
      """
      SELECT * FROM code.create_job_result('11', 'minion', 'FAILURE', '{}')
      """
      And I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: ERROR
      done_count: 0
      errors_count: 1
      total_count: 4
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
      Then it returns "4" rows matches
      """
      - id: 1
        fqdn: minion
        status: ERROR
      - id: 2
        fqdn: minion2
        status: RUNNING
      - id: 3
        fqdn: minion3
        status: CANCELED
      - id: 4
        fqdn: minion4
        status: CANCELED
      """

    Scenario: Create shipment with 3 commands
      Given "minion1" minion in "testing" group
      And "minion2" minion in "testing" group
      And "minion3" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(ARRAY[ ('c1', '{}', NULL),
                                               ('c2', '{}', NULL),
                                               ('c3', '{}', NULL) ] AS code.command_def[]),
          i_fqdns               => '{"minion2", "minion1", "minion3"}',
          i_parallel            => 2,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it success
      And it returns one row matches
      """
      other_count: 3
      done_count: 0
      errors_count: 0
      total_count: 3
      """
      When I execute query
      """
      SELECT fqdn, type
        FROM code.get_commands_for_dispatch('master', NULL)
       ORDER BY id
      """
      Then it returns "2" rows matches
      """
      - fqdn: 'minion2'
        type: 'c1'
      - fqdn: 'minion1'
        type: 'c1'
      """

    Scenario: Create shipment on non-existent minion
      Given "minion1" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('cmd', NULL, NULL) AS code.command_def)],
          i_fqdns               => '{"minion1", "minion2", "minion3"}',
          i_parallel            => 3,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it fail with error "Not all commands created, got 1, need 3. Probably {minion2,minion3} minions missing, {} minions deleted" and code "MDD05"

    @MDB-15128
    Scenario: Create shipment on deleted minion
      Given "minion1" minion in "testing" group
      And "minion2" minion in "testing" group
      And "minion3" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.delete_minion(
          i_fqdn => 'minion3'
      )
      """
      Then it success
      When I execute query
        """
        SELECT * FROM code.create_shipment(
            i_commands            => ARRAY[cast(('cmd', NULL, NULL) AS code.command_def)],
            i_fqdns               => '{"minion1", "minion2", "minion3"}',
            i_parallel            => 3,
            i_stop_on_error_count => 1,
            i_timeout             => INTERVAL '1 hour')
        """
      Then it fail with error "Not all commands created, got 2, need 3. Probably {} minions missing, {minion3} minions deleted" and code "MDD05"

    Scenario: Create shipment with empty fqdns
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('cmd', NULL, NULL) AS code.command_def)],
          i_fqdns               => NULL,
          i_parallel            => 4,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it fail with error "i_fqdns should not be empty"

    Scenario: Create shipment with empty commands
      Given "minion1" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => cast(NULL AS code.command_def[]),
          i_fqdns               => '{"minion1"}',
          i_parallel            => 1,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it fail with error "i_commands should not be empty"

    Scenario: Create shipment with parallel greater then fqdns count
      Given "minion1" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('cmd', NULL, NULL) AS code.command_def)],
          i_fqdns               => '{"minion1"}',
          i_parallel            => 2,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it fail with error "i_parallel should be less than or equal to i_fqdns cardinality"

    Scenario: Create shipment with one fqdn skipped
      Given "minion1" minion in "testing" group
      And "minion2" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type', '{"test=True"}', '40 minutes') AS code.command_def)],
          i_fqdns               => '{"minion1", "minion2"}',
          i_skip_fqdns          => '{"minion1"}',
          i_parallel            => 2,
          i_stop_on_error_count => 2,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type", "arguments": ["test=True"], "timeout": "00:40:00"}]
      fqdns:
       - minion2
      status: INPROGRESS
      parallel: 1
      stop_on_error_count: 1
      other_count: 1
      done_count: 0
      errors_count: 0
      total_count: 1
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type", "arguments": ["test=True"], "timeout": "00:40:00"}]
      fqdns:
       - minion2
      status: INPROGRESS
      parallel: 1
      stop_on_error_count: 1
      other_count: 1
      done_count: 0
      errors_count: 0
      total_count: 1
      """

    Scenario: Create shipment with all fqdns skipped
      Given "minion1" minion in "testing" group
      And "minion2" minion in "testing" group
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => ARRAY[cast(('type', '{"test=True"}', '40 minutes') AS code.command_def)],
          i_fqdns               => '{"minion1", "minion2"}',
          i_skip_fqdns          => '{"minion1", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 2,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type", "arguments": ["test=True"], "timeout": "00:40:00"}]
      fqdns:
       - minion1
       - minion2
      status: DONE
      parallel: 2
      stop_on_error_count: 2
      other_count: 0
      done_count: 2
      errors_count: 0
      total_count: 2
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      shipment_id: 1
      commands: [{"type": "type", "arguments": ["test=True"], "timeout": "00:40:00"}]
      fqdns:
       - minion1
       - minion2
      status: DONE
      parallel: 2
      stop_on_error_count: 2
      other_count: 0
      done_count: 2
      errors_count: 0
      total_count: 2
      """
