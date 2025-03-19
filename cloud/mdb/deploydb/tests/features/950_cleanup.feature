Feature: Cleanup API
    Background: Database at last migration and ...
      Given database at last migration
      And "testing" group
      And open alive "master" master in "testing" group
      And "minion" minion in "testing" group

    Scenario: Create 2 job results and remove the oldest without valid job
      When I execute query
        """
        SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{"a": 1}', '2019-03-05T00:00:00.0+03:00')
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
        SELECT * FROM code.create_job_result('456', 'minion', 'TIMEOUT', '{"c": 3}')
        """
      Then it returns one row matches
        """
        id: 2
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
        """
      When I execute query
        """
        SELECT * FROM code.cleanup_unbound_job_results('30 days'::interval, 10)
        """
      Then it returns one row matches
        """
        cleanup_unbound_job_results: 1
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
      Then it returns one row matches
        """
        id: 2
        order_id: 1
        ext_id: '456'
        fqdn: 'minion'
        status: TIMEOUT
        result: {'c': 3}
        """

    Scenario: Create 2 job results with valid job and do nothing
      Given "type" shipment on "minion"
      When I execute query
        """
        SELECT * FROM code.create_job('123', 1)
        """
      When I execute query
        """
        SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{"a": 1}', '2019-03-05T00:00:00.0+03:00')
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
        SELECT * FROM code.create_job('456', 1)
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
        SELECT * FROM code.cleanup_unbound_job_results('30 days'::interval, 10)
        """
      Then it returns one row matches
        """
        cleanup_unbound_job_results: 0
        """


    Scenario: Create 2 shipment and cleanup the old
      Given "type" shipment on "minion"
      When I execute query
        """
        SELECT * FROM code.create_job('123', 1)
        """
      When I execute query
        """
        SELECT * FROM code.create_job_result('123', 'minion', 'SUCCESS', '{"a": 1}', '2019-03-05T00:00:00.0+03:00')
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
        SELECT * FROM code.create_job('456', 1)
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
        SELECT * FROM code.cleanup_shipments('30 days'::interval, 10)
        """
      Then it returns one row matches
        """
        cleanup_shipments: 1
        """

    Scenario: Cleanup minion without undegroud data
      Given "minion2" minion in "testing" group
      And successfully executed query
        """
        SELECT * FROM code.create_shipment(
            i_commands            => ARRAY[cast(('type', '{"a", "b=c"}', '1 minute') AS code.command_def)],
            i_fqdns               => '{"minion2"}',
            i_parallel            => 1,
            i_stop_on_error_count => 0,
            i_timeout             => INTERVAL '0 hour')
        """
      And successfully executed query
      """
      SELECT * FROM code.create_job('11', 1)
      """
      When I execute query
        """
        SELECT * FROM code.cleanup_minions_without_jobs('0 days'::interval, 10)
        """
      Then it returns one row matches
        """
        cleanup_minions_without_jobs: 0
        """
      When successfully executed query
        """
        SELECT * FROM code.create_job_result('11', 'minion2', 'FAILURE', '{}')
        """
      When I execute query
        """
        SELECT * FROM code.cleanup_shipments('0 days'::interval, 10)
        """
      Then it returns one row matches
        """
        cleanup_shipments: 1
        """
      When I execute query
        """
        SELECT * FROM code.delete_minion('minion2')
        """
      Then it returns one row matches
        """
        deleted: true 
        deploy_group: 'testing' 
        fqdn: 'minion2'
        """
      When I execute query
        """
        SELECT * FROM code.cleanup_minions_without_jobs('0 days'::interval, 10)
        """
      Then it returns one row matches
        """
        cleanup_minions_without_jobs: 1
        """
