Feature: Jobs API
    Background: Database at last migration and ...
      Given database at last migration
      And "testing" group
      And open alive "master" master in "testing" group
      And "minion" minion in "testing" group

    Scenario: Create job and get it
      Given "type" shipment on "minion"
      When I execute query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      Then it returns one row matches
      """
      id: 1
      ext_id: '123'
      command_id: 1
      status: RUNNING
      """

    Scenario: Fail to create job for unavailable command
      Given "minion2" minion in "testing" group
      And "type" shipment on "minion, minion2"
      When I execute query
      """
      SELECT * FROM code.create_job('123', 2)
      """
    Then it fail with error "Command status is not AVAILABLE, id 2" and code "MDD05"

    Scenario: Create job successful result and validate job, command and deploy status
      Given "type" shipment on "minion"
      And successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      When I execute query
      """
      SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{}')
      """
      Then it returns one row matches
      """
      ext_id: '123'
      fqdn: minion
      status: SUCCESS
      result: {}
      """
      When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
      Then it returns one row matches
      """
      ext_id: '123'
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_command(1)
      """
      Then it returns one row matches
      """
      id: 1
      type: type
      fqdn: minion
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      shipment_id: 1
      fqdns:
        - minion
      status: DONE
      parallel: 1
      stop_on_error_count: 0
      """

  Scenario: Check running jobs
    Given "type" shipment on "minion"
    When I execute query
      """
      SELECT * FROM code.running_job_missing('123', 'minion', 3)
      """
    Then it fail with error "Unable to find job ext id 123, fqdn minion" and code "MDD03"
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('master', INTERVAL '30 seconds', 100)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('unknown', INTERVAL '30 seconds', 100)
      """
    Then it returns nothing
    Given successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
    When I execute query
      """
      SELECT * FROM deploy.jobs WHERE created_at = last_running_check_at
      """
    Then it returns one row matches
      """
      ext_job_id: '123'
      """
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('master', INTERVAL '30 seconds', 100)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM deploy.jobs WHERE created_at = last_running_check_at
      """
    Then it returns one row matches
      """
      ext_job_id: '123'
      """
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('unknown', INTERVAL '30 seconds', 100)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM deploy.jobs WHERE created_at = last_running_check_at
      """
    Then it returns one row matches
      """
      ext_job_id: '123'
      """
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('master', INTERVAL '0 seconds', 100)
      """
    Then it returns one row matches
      """
      ext_job_id: '123'
      minion: minion
      """
    When I execute query
      """
      SELECT * FROM deploy.jobs WHERE created_at = last_running_check_at
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('unknown', INTERVAL '0 seconds', 100)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM deploy.jobs WHERE created_at = last_running_check_at
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: RUNNING
      """
    When I execute query
      """
      SELECT * FROM code.running_job_missing('123', 'minion', 3)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: RUNNING
      """
    When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: RUNNING
      """
    When I execute query
      """
      SELECT * FROM code.running_job_missing('123', 'minion', 3)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: RUNNING
      """
    When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: RUNNING
      """
    When I execute query
      """
      SELECT * FROM code.running_job_missing('123', 'minion', 3)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: ERROR
      """
    When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
    Then it returns one row matches
      """
      ext_id: '123'
      status: ERROR
      """
    When I execute query
      """
      SELECT * FROM code.get_job_result(1)
      """
    Then it returns one row matches
      """
      id: 1
      status: NOTRUNNING
      """
    When I execute query
      """
      SELECT * FROM code.get_running_jobs_and_mark_them('unknown', INTERVAL '0 seconds', 100)
      """
    Then it returns nothing

    Scenario: Create job result at first and then create job
      Given "type" shipment on "minion"
      When I execute query
      """
      SELECT * FROM code.create_job_result('4321', 'minion', 'SUCCESS', '{}')
      """
      Then it returns one row matches
      """
      ext_id: '4321'
      fqdn: minion
      status: SUCCESS
      result: {}
      """
      When I execute query
      """
      SELECT * FROM code.create_job('4321', 1)
      """
      Then it returns one row matches
      """
      ext_id: '4321'
      status: DONE
      command_id: 1
      """
      When I execute query
      """
      SELECT * FROM deploy.job_results
      """
      Then it returns one row matches
      """
      fqdn: minion
      status: SUCCESS
      ext_job_id: '4321'
      result: {}
      """
      When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
      Then it returns one row matches
      """
      ext_id: '4321'
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_command(1)
      """
      Then it returns one row matches
      """
      id: 1
      type: type
      fqdn: minion
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      shipment_id: 1
      fqdns:
        - minion
      status: DONE
      parallel: 1
      stop_on_error_count: 0
      """

    Scenario: Create job result at first and then create shipment and job
      When I execute query
      """
      SELECT * FROM code.create_job_result('3321', 'minion', 'SUCCESS', '{}')
      """
      Then it returns one row matches
      """
      ext_id: '3321'
      fqdn: minion
      status: SUCCESS
      result: {}
      """
      When I execute query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => code.commands_def_from_json('[{"type":"type"}]'),
          i_fqdns               => '{"minion"}',
          i_parallel            => 1,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      Then it returns one row matches
      """
      shipment_id: 1
      fqdns:
        - minion
      status: INPROGRESS
      parallel: 1
      stop_on_error_count: 0
      """
      When I execute query
      """
      SELECT * FROM code.create_job('3321', 1)
      """
      Then it returns one row matches
      """
      ext_id: '3321'
      status: DONE
      command_id: 1
      """
      When I execute query
      """
      SELECT * FROM deploy.job_results
      """
      Then it returns one row matches
      """
      fqdn: minion
      status: SUCCESS
      ext_job_id: '3321'
      result: {}
      """
      When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
      Then it returns one row matches
      """
      ext_id: '3321'
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_command(1)
      """
      Then it returns one row matches
      """
      id: 1
      type: type
      fqdn: minion
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      shipment_id: 1
      fqdns:
        - minion
      status: DONE
      parallel: 1
      stop_on_error_count: 0
      """

    Scenario: Create 2 jobs and get their listing
      Given "minion2" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => code.commands_def_from_json('[{"type":"type"}]'),
          i_fqdns               => '{"minion", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job_result('123', 'minion', 'FAILURE', '{}')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('456', 2)
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => 1,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => 2,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => 'minion',
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => 'wrong',
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => '123',
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => '456',
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => 'ERROR',
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => 'RUNNING',
        i_limit           => 100,
        i_last_job_id     => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 1,
        i_last_job_id     => NULL
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 1,
        i_last_job_id     => 1
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 1,
        i_last_job_id     => 2
      )
      """
      Then it returns nothing
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 2,
        i_last_job_id     => NULL
      )
      """
      Then it returns "2" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 2,
        i_last_job_id     => 1
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """

    Scenario: Create 2 jobs and get their listing in two way
      Given "minion2" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => code.commands_def_from_json('[{"type":"type"}]'),
          i_fqdns               => '{"minion", "minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job_result('123', 'minion', 'FAILURE', '{}')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('456', 2)
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => NULL,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => 1,
        i_ascending       => true
      )
      """
      Then it returns "1" rows matches
      """
      - id: 2
        ext_id: '456'
        command_id: 2
        status: RUNNING
      """
      When I execute query
      """
      SELECT * FROM code.get_jobs(
        i_shipment_id     => 1,
        i_fqdn            => NULL,
        i_ext_job_id      => NULL,
        i_status          => NULL,
        i_limit           => 100,
        i_last_job_id     => 2,
        i_ascending       => false
      )
      """
      Then it returns "1" rows matches
      """
      - id: 1
        ext_id: '123'
        command_id: 1
        status: ERROR
      """

  Scenario: Create 4 job results and get their listing
    When I execute query
      """
      SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{"a": 1}')
      """
    Then it returns one row matches
      """
      id: 1
      order_id: 1
      ext_id: '123'
      fqdn: 'minion'
      status: SUCCESS
      result: {'a': 1}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_result(1)
      """
    Then it returns one row matches
      """
      id: 1
      order_id: 1
      ext_id: '123'
      fqdn: 'minion'
      status: SUCCESS
      result: {'a': 1}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_result(2)
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.create_job_result('123', 'minion2', 'FAILURE', '{"b": 2}')
      """
    Then it returns one row matches
      """
      id: 2
      order_id: 1
      ext_id: '123'
      fqdn: 'minion2'
      status: FAILURE
      result: {'b': 2}
      """
    When I execute query
      """
      SELECT * FROM code.create_job_result('456', 'minion', 'TIMEOUT', '{"c": 3}')
      """
    Then it returns one row matches
      """
      id: 3
      order_id: 1
      ext_id: '456'
      fqdn: 'minion'
      status: TIMEOUT
      result: {'c': 3}
      """
    When I execute query
      """
      SELECT * FROM code.create_job_result('456', 'minion', 'SUCCESS', '{"d": 4}')
      """
    Then it returns one row matches
      """
      id: 5
      order_id: 2
      ext_id: '456'
      fqdn: 'minion'
      status: SUCCESS
      result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "4" rows matches
      """
      - id: 1
        order_id: 1
        ext_id: '123'
        fqdn: 'minion'
        status: SUCCESS
        result: {'a': 1}
      - id: 2
        order_id: 1
        ext_id: '123'
        fqdn: 'minion2'
        status: FAILURE
        result: {'b': 2}
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 1,
        i_last_job_result_id  => 1,
        i_ascending           => true
      )
      """
    Then it returns "1" rows matches
      """
      - id: 2
        order_id: 1
        ext_id: '123'
        fqdn: 'minion2'
        status: FAILURE
        result: {'b': 2}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 1,
        i_last_job_result_id  => 2,
        i_ascending           => false
      )
      """
    Then it returns "1" rows matches
      """
      - id: 1
        order_id: 1
        ext_id: '123'
        fqdn: 'minion'
        status: SUCCESS
        result: {'a': 1}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => NULL,
        i_status              => 'FAILURE',
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "1" rows matches
      """
      - id: 2
        order_id: 1
        ext_id: '123'
        fqdn: 'minion2'
        status: FAILURE
        result: {'b': 2}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 2,
        i_last_job_result_id  => 4
      )
      """
    Then it returns "1" rows matches
      """
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 2,
        i_last_job_result_id  => 5
      )
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "3" rows matches
      """
      - id: 1
        order_id: 1
        ext_id: '123'
        fqdn: 'minion'
        status: SUCCESS
        result: {'a': 1}
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => NULL,
        i_fqdn                => 'minion2',
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "1" rows matches
      """
      - id: 2
        order_id: 1
        ext_id: '123'
        fqdn: 'minion2'
        status: FAILURE
        result: {'b': 2}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '123',
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - id: 1
        order_id: 1
        ext_id: '123'
        fqdn: 'minion'
        status: SUCCESS
        result: {'a': 1}
      - id: 2
        order_id: 1
        ext_id: '123'
        fqdn: 'minion2'
        status: FAILURE
        result: {'b': 2}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => NULL,
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '123',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "1" rows matches
      """
      - id: 1
        order_id: 1
        ext_id: '123'
        fqdn: 'minion'
        status: SUCCESS
        result: {'a': 1}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '123',
        i_fqdn                => 'minion2',
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "1" rows matches
      """
      - id: 2
        order_id: 1
        ext_id: '123'
        fqdn: 'minion2'
        status: FAILURE
        result: {'b': 2}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion2',
        i_status              => NULL,
        i_limit               => 100,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 1,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "1" rows matches
      """
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 1,
        i_last_job_result_id  => 1
      )
      """
    Then it returns "1" rows matches
      """
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 1,
        i_last_job_result_id  => 3
      )
      """
    Then it returns "1" rows matches
      """
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 1,
        i_last_job_result_id  => 5
      )
      """
    Then it returns nothing
    When I execute query
      """
      SELECT * FROM code.get_job_results(
        i_ext_job_id          => '456',
        i_fqdn                => 'minion',
        i_status              => NULL,
        i_limit               => 2,
        i_last_job_result_id  => NULL
      )
      """
    Then it returns "2" rows matches
      """
      - id: 3
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
      - id: 5
        order_id: 2
        ext_id: '456'
        fqdn: 'minion'
        status: SUCCESS
        result: {'d': 4}
      """

    Scenario: Job result for wrong job id
      Given "type" shipment on "minion"
      And successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      When I execute query
      """
      SELECT * FROM code.create_job_result('123', 'wrong_minion', 'SUCCESS', '{}')
      """
      Then it returns one row matches
      """
      ext_id: '123'
      fqdn: wrong_minion
      status: SUCCESS
      result: {}
      """

    Scenario: Job result for wrong fqdn
      Given "type" shipment on "minion"
      And successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      When I execute query
      """
      SELECT * FROM code.create_job_result('321', 'minion', 'SUCCESS', '{}')
      """
      Then it returns one row matches
      """
      ext_id: '321'
      fqdn: minion
      status: SUCCESS
      result: {}
      """

    Scenario: Create 2 jobs with same ext_id
      Given "minion2" minion in "testing" group
      And successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => code.commands_def_from_json('[{"type":"type"}]'),
          i_fqdns               => '{"minion","minion2"}',
          i_parallel            => 2,
          i_stop_on_error_count => 0,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('10', 1)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('10', 2)
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job_result('10', 'minion', 'SUCCESS', '{}')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job_result('10', 'minion2', 'FAILURE', '{}')
      """
      When I execute query
      """
      SELECT * FROM code.get_job(1)
      """
      Then it returns one row matches
      """
      ext_id: '10'
      status: DONE
      """
      When I execute query
      """
      SELECT * FROM code.get_job(2)
      """
      Then it returns one row matches
      """
      ext_id: '10'
      status: ERROR
      """

    @MDB-4299
    Scenario: Save second job result
        . should save second job_results
        . but don't touch command.STATUS and shipment.STATUS
      Given successfully executed query
      """
      SELECT * FROM code.create_shipment(
          i_commands            => code.commands_def_from_json('[{"type":"type"}]'),
          i_fqdns               => '{"minion"}',
          i_parallel            => 1,
          i_stop_on_error_count => 1,
          i_timeout             => INTERVAL '1 hour')
      """
      And successfully executed query
      """
      SELECT * FROM code.create_job('123', 1)
      """
      When I execute query
      """
      SELECT * FROM code.create_job_result('123', 'minion', 'FAILURE', '{"why": "timeout"}')
      """
      Then it returns one row matches
      """
      status: FAILURE
      result: {"why": "timeout"}
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
      SELECT status FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: ERROR
      """
      When I execute query
      """
      SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{"completed": true}')
      """
      Then it returns one row matches
      """
      status: SUCCESS
      result: {"completed": true}
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
      SELECT status FROM code.get_shipment(1)
      """
      Then it returns one row matches
      """
      status: ERROR
      """

    Scenario: Create job that does not timeout
      Given "type" shipment on "minion"
      And successfully executed query
        """
        SELECT * FROM code.create_job('123', 1)
        """
      When I execute query
        """
        SELECT * FROM code.timeout_jobs(100)
        """
      Then it returns nothing
      When I execute query
        """
        SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{}')
        """
      Then it returns one row matches
        """
        ext_id: '123'
        fqdn: minion
        status: SUCCESS
        result: {}
        """
      When I execute query
        """
        SELECT * FROM code.timeout_jobs(100)
        """
      Then it returns nothing

    Scenario: Create job that times out
      Given successfully executed query
        """
        SELECT * FROM code.create_shipment(
            i_commands            => ARRAY[cast(('type', '{}', INTERVAL '1 second') AS code.command_def)],
            i_fqdns               => '{"minion"}',
            i_parallel            => 1,
            i_stop_on_error_count => 0,
            i_timeout             => INTERVAL '2 hours')
        """
      And successfully executed query
        """
        SELECT * FROM code.create_job('123', 1)
        """
      And successfully executed query
        """
        UPDATE deploy.jobs SET created_at = created_at - INTERVAL '1 minute' WHERE ext_job_id = '123'
        """
      When I execute query
        """
        SELECT * FROM code.timeout_jobs(100)
        """
      Then it returns one row matches
        """
        id: 1
        ext_id: '123'
        """
      When I execute query
        """
        SELECT * FROM code.timeout_jobs(100)
        """
      Then it returns nothing

    Scenario: Valid job result coords
      Given "type" shipment on "minion"
      And successfully executed query
        """
         SELECT * FROM code.create_job('123', 1)
        """
      And successfully executed query
        """
        SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{}')
        """
      When I execute query
        """
        SELECT * FROM code.get_job_result_coords('123', 'minion')
        """
      Then it returns one row matches
        """
        shipment_id: 1
        command_id: 1
        job_id: 1
        tracing: NULL
        """

    Scenario: Invalid job result coords
      Given "type" shipment on "minion"
      And successfully executed query
        """
         SELECT * FROM code.create_job('123', 1)
        """
      And successfully executed query
        """
        SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{}')
        """
      When I execute query
        """
        SELECT * FROM code.get_job_result_coords('invalid', 'minion')
        """
      Then it returns nothing
