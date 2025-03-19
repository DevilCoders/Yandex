Feature: Commands API

    Background: Database with group, master and minion
      Given database at last migration
      And "testing" group
      And open alive "master" master in "testing" group
      And "minion" minion in "testing" group

    Scenario: Create command and get it
      Given "type" shipment on "minion"
      When I execute query
      """
      SELECT * FROM code.get_command(1)
      """
      Then it returns one row matches
      """
      id: 1
      shipment_id: 1
      type: type
      fqdn: minion
      status: AVAILABLE
      """

    Scenario: Create 2 commands
      Given "minion2" minion in "testing" group
      And "type" shipment on "minion, minion2"
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
      Then it returns "2" rows matches
      """
      - id: 1
        shipment_id: 1
        type: type
        fqdn: minion
        status: AVAILABLE
      - id: 2
        shipment_id: 1
        type: type
        fqdn: minion2
        status: BLOCKED
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => 1,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - id: 1
        shipment_id: 1
        type: type
        fqdn: minion
        status: AVAILABLE
      - id: 2
        shipment_id: 1
        type: type
        fqdn: minion2
        status: BLOCKED
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => 2,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => 'minion',
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        shipment_id: 1
        type: type
        fqdn: minion
        status: AVAILABLE
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => 'fake',
        i_status          => NULL,
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => 'AVAILABLE',
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        shipment_id: 1
        type: type
        fqdn: minion
        status: AVAILABLE
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => 'BLOCKED',
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        shipment_id: 1
        type: type
        fqdn: minion2
        status: BLOCKED
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => 'ERROR',
        i_limit           => 100,
        i_last_command_id => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 1,
        i_last_command_id => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        shipment_id: 1
        type: type
        fqdn: minion
        status: AVAILABLE
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 1,
        i_last_command_id => 1
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        shipment_id: 1
        type: type
        fqdn: minion2
        status: BLOCKED
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 1,
        i_last_command_id => 2
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 2,
        i_last_command_id => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - id: 1
        shipment_id: 1
        type: type
        fqdn: minion
        status: AVAILABLE
      - id: 2
        shipment_id: 1
        type: type
        fqdn: minion2
        status: BLOCKED
      """
      When I execute query
      """
      SELECT * FROM code.get_commands(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_status          => NULL,
        i_limit           => 2,
        i_last_command_id => 1
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        shipment_id: 1
        type: type
        fqdn: minion2
        status: BLOCKED
      """

    Scenario: Get commands for dispatch
      Given "minion2" minion in "testing" group
      And "type" shipment on "minion, minion2"
      When I execute query
      """
      SELECT * FROM code.get_commands_for_dispatch('unknown', 1)
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_commands_for_dispatch('master', 1)
      """
      Then it returns one row matches
      """
      id: 1
      shipment_id: 1
      status: AVAILABLE
      fqdn: minion
      """
      When I execute query
      """
      SELECT * FROM code.get_commands_for_dispatch('master', 1, 1)
      """
      Then it returns one row matches
      """
      id: 1
      shipment_id: 1
      status: AVAILABLE
      fqdn: minion
      """
      When I execute query
      """
      SELECT * FROM code.get_commands_for_dispatch('master', 1, 0)
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.command_dispatch_failed(1)
      """
      Then it returns one row matches
      """
      id: 1
      shipment_id: 1
      status: AVAILABLE
      fqdn: minion
      """
      When I execute query
      """
      SELECT * FROM code.get_commands_for_dispatch('unknown', 1)
      """
      Then it returns nothing
      When I successfully execute query
      """
      UPDATE deploy.commands SET last_dispatch_attempt_at = now() - INTERVAL '1 hour' WHERE command_id = 1
      """
      When I execute query
      """
      SELECT * FROM code.get_commands_for_dispatch('master', 1)
      """
      Then it returns one row matches
      """
      id: 1
      shipment_id: 1
      status: AVAILABLE
      fqdn: minion
      """
