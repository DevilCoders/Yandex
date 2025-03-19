Feature: Minions API

  Background: Fresh database
    Given database at last migration

  Scenario: Create minion
    Given "testing" group
    And open alive "open.master" master in "testing" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'created.minion',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: null
    registered: false
    """

  Scenario: Create minion with 2 masters in group
    Given "testing" group
    And open alive "open.master" master in "testing" group
    And open alive "open.master_2" master in "testing" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'created.minion',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: null
    registered: false
    """

  Scenario: Create minion with 2 masters in group (one closed)
    Given "testing" group
    And open alive "open.master" master in "testing" group
    And closed alive "closed.master" master in "testing" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'created.minion',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: null
    registered: false
    """

  Scenario: Create minion with 2 groups
    Given "testing" group
    And "testing_2" group
    And open alive "open.master" master in "testing" group
    And open alive "open.master_2" master in "testing_2" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'created.minion',
        i_deploy_group  => 'testing_2',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing_2
    master_fqdn: open.master_2
    auto_reassign: true
    pub_key: null
    registered: false
    """

  Scenario: Create, register, unregister and delete minion
    Given "testing" group
    And open alive "open.master" master in "testing" group
    And closed alive "closed.master" master in "testing" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'created.minion',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: null
    registered: false
    deleted: false
    """
    When I execute query
    """
    SELECT * FROM code.register_minion(
        i_fqdn     => 'created.minion',
        i_pub_key  => 'key'
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: 'key'
    registered: true
    deleted: false
    """
    When I execute query
    """
    SELECT * FROM code.get_minion('created.minion')
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: 'key'
    registered: true
    deleted: false
    """
    When I execute query
    """
    SELECT * FROM code.get_minions_by_master(
        i_master_fqdn     => 'open.master',
        i_limit           => 100,
        i_last_minion_id  => NULL
    )
    """
    Then it returns "1" rows matches
    """
    - fqdn: created.minion
      deleted: false
    """
    When I execute query
    """
    SELECT * FROM code.unregister_minion(
        i_fqdn     => 'created.minion'
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: 'key'
    registered: true
    deleted: false
    """
    When I execute query
    """
    SELECT * FROM code.delete_minion(
        i_fqdn => 'created.minion'
    )
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deploy_group: testing
    master_fqdn: open.master
    auto_reassign: true
    pub_key: 'key'
    registered: true
    deleted: true
    """
    When I execute query
    """
    SELECT * FROM code.delete_minion(
        i_fqdn => 'created.minion'
    )
    """
    Then it returns nothing
    When I execute query
    """
    SELECT * FROM code.get_minions_by_master(
        i_master_fqdn     => 'open.master',
        i_limit           => 100,
        i_last_minion_id  => NULL
    )
    """
    Then it returns "1" rows matches
    """
    - fqdn: created.minion
      deleted: true
    """
    When I execute query
    """
    SELECT * FROM code.get_minion('created.minion')
    """
    Then it returns one row matches
    """
    fqdn: created.minion
    deleted: true
    """
    When I execute query
    """
    SELECT
        change_type,
        old_row,
        old_row->'fqdn' AS old_row_fqdn,
        old_row->'auto_reassign' AS old_row_auto_reassign,
        old_row->'pub_key' AS old_row_pub_key,
        old_row->'master_id' AS old_row_master_id,
        new_row,
        new_row->'fqdn' AS new_row_fqdn,
        new_row->'auto_reassign' AS new_row_auto_reassign,
        new_row->'pub_key' AS new_row_pub_key,
        new_row->'master_id' AS new_row_master_id
     FROM deploy.minions_change_log
    """
    Then it returns "4" rows matches
    """
    - change_type: create
      old_row: null
      new_row_fqdn: created.minion
      new_row_auto_reassign: true
      new_row_pub_key: null
      new_row_master_id: 1
    - change_type: register
      old_row_fqdn: created.minion
      old_row_auto_reassign: true
      old_row_pub_key: null
      old_row_master_id: 1
      new_row_fqdn: created.minion
      new_row_auto_reassign: true
      new_row_pub_key: key
      new_row_master_id: 1
    - change_type: unregister
      old_row_fqdn: created.minion
      old_row_auto_reassign: true
      old_row_pub_key: key
      old_row_master_id: 1
      new_row_fqdn: created.minion
      new_row_auto_reassign: true
      new_row_pub_key: key
      new_row_master_id: 1
    - change_type: delete
      old_row_fqdn: created.minion
      old_row_auto_reassign: true
      old_row_pub_key: key
      old_row_master_id: 1
      new_row: null
    """

  Scenario: Create minion choice master with less minions
    Given "testing" group
    And open alive "master1" master in "testing" group
    And open alive "master2" master in "testing" group
    When I execute query
    """
    SELECT master_fqdn FROM code.create_minion(
        i_fqdn          => 'minion1',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    master_fqdn: 'master1'
    """
    When I execute query
    """
    SELECT master_fqdn FROM code.create_minion(
        i_fqdn          => 'minion2',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    master_fqdn: 'master2'
    """
    When I execute query
    """
    SELECT master_fqdn FROM code.create_minion(
        i_fqdn          => 'minion3',
        i_deploy_group  => 'testing',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    master_fqdn: 'master1'
    """
    When I execute query
    """
    SELECT id, fqdn, master_fqdn
      FROM code.get_minions_by_master(
        i_master_fqdn     => 'master2',
        i_limit           => 100,
        i_last_minion_id  => NULL
    )
    ORDER BY fqdn
    """
    Then it returns one row matches
    """
    id: 2
    fqdn: 'minion2'
    master_fqdn: 'master2'
    """
    When I execute query
    """
    SELECT id, fqdn, master_fqdn
      FROM code.get_minions_by_master(
        i_master_fqdn     => 'master1',
        i_limit           => 100,
        i_last_minion_id  => NULL
    )
    ORDER BY fqdn
    """
    Then it returns "2" rows matches
    """
    - id: 1
      fqdn: 'minion1'
      master_fqdn: 'master1'
    - id: 3
      fqdn: 'minion3'
      master_fqdn: 'master1'
    """
    When I execute query
    """
    SELECT id, fqdn, master_fqdn
      FROM code.get_minions_by_master(
        i_master_fqdn     => 'master1',
        i_limit           => 1,
        i_last_minion_id  => NULL
    )
    ORDER BY fqdn
    """
    Then it returns one row matches
    """
    id: 1
    fqdn: 'minion1'
    master_fqdn: 'master1'
    """
    When I execute query
    """
    SELECT id, fqdn, master_fqdn
      FROM code.get_minions_by_master(
        i_master_fqdn     => 'master1',
        i_limit           => 1,
        i_last_minion_id  => 1
    )
    ORDER BY fqdn
    """
    Then it returns one row matches
    """
    id: 3
    fqdn: 'minion3'
    master_fqdn: 'master1'
    """

  Scenario: Create minion in non existed group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'bad.minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    Then it fail with error "Unable to find group with name dev" and code "MDD03"

  Scenario: Create minion in group without open masters
    Given "dev" group
    And closed alive "closed.master" master in "dev" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'bad.minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    Then it fail with error "Unable to find open master in group dev" and code "MDD04"

  Scenario: Create minion same fqdn
    Given "dev" group
    And open alive "closed.master" master in "dev" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'bad.minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'bad.minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    Then it fail with error "duplicate key value violates unique constraint"

  Scenario: Create minion with upsert and update it with same group
    Given "testing" group
    And "prod" group
    And open alive "master" master in "prod" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion',
        i_deploy_group  => 'prod',
        i_auto_reassign => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'master'
    deploy_group: 'prod'
    auto_reassign: false
    """
    When I execute query
    """
    SELECT * FROM code.upsert_minion(
        i_fqdn          => 'minion',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'master'
    deploy_group: 'prod'
    auto_reassign: true
    """

  Scenario: Create minion with upsert and update it with different group
    Given "testing" group
    And "prod" group
    And "dev" group
    And open alive "prodmaster" master in "prod" group
    And open alive "devmaster" master in "dev" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'devmaster'
    deploy_group: 'dev'
    auto_reassign: false
    """
    When I execute query
    """
    SELECT * FROM code.upsert_minion(
        i_fqdn          => 'minion',
        i_deploy_group  => 'prod',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'prodmaster'
    deploy_group: 'prod'
    auto_reassign: true
    """

  Scenario: Create minion with upsert and update it with different master
    Given "testing" group
    And "prod" group
    And "dev" group
    And open alive "prodmaster" master in "prod" group
    And open alive "devmaster" master in "dev" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'devmaster'
    deploy_group: 'dev'
    auto_reassign: false
    """
    When I execute query
    """
    SELECT * FROM code.upsert_minion(
        i_fqdn          => 'minion',
        i_master        => 'prodmaster',
        i_auto_reassign => true
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'prodmaster'
    deploy_group: 'prod'
    auto_reassign: true
    """

  Scenario: Create minion with upsert and fail if update both group and master
    Given "testing" group
    And "dev" group
    And open alive "devmaster" master in "dev" group
    When I execute query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'minion'
    master_fqdn: 'devmaster'
    deploy_group: 'dev'
    auto_reassign: false
    """
    When I execute query
    """
    SELECT * FROM code.upsert_minion(
        i_fqdn          => 'minion',
        i_deploy_group  => 'prod',
        i_master        => 'prodmaster',
        i_auto_reassign => true
    )
    """
    Then it fail with error "Unable to set both master and group." and code "MDD06"

  Scenario: Register non-existant minion
    When I execute query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'non-existed-minion',
        i_pub_key => 'blah'
    )
    """
    Then it fail with error "Unable to find minion non-existed-minion" and code "MDD03"

  Scenario: Register minion with expired register_until
    Given "dev" group
    And open alive "master" master in "dev" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion-fqdn',
        i_deploy_group  => 'dev',
        i_auto_reassign => true,
        i_register_ttl  => '-1 days'::interval
    )
    """
    When I execute query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah'
    )
    """
    Then it fail with error "Unable to register minion minion-fqdn: registration timeout" and code "MDD02"

  Scenario: Register already registered minion and not allowed to reregister
    Given "dev" group
    And open alive "master" master in "dev" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion-fqdn',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah'
    )
    """
    When I execute query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah-blah'
    )
    """
    Then it fail with error "Unable to register minion minion-fqdn: already registered and not allowed to reregister" and code "MDD01"

  Scenario: Reregister already registered minion
    Given "dev" group
    And open alive "master" master in "dev" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion-fqdn',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah'
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.unregister_minion(
        i_fqdn    => 'minion-fqdn'
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah-blah'
    )
    """

  Scenario: Register already registered minion with same key
    Given "dev" group
    And open alive "master" master in "dev" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion-fqdn',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah'
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.unregister_minion(
        i_fqdn    => 'minion-fqdn'
    )
    """
    When I execute query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'minion-fqdn',
        i_pub_key => 'blah'
    )
    """
    Then it fail with error "Unable to register minion minion-fqdn: already registered with same key" and code "MDD01"

  Scenario: Unregister non-existant minion
    When I execute query
    """
    SELECT * FROM code.unregister_minion(
        i_fqdn    => 'non-existed-minion'
    )
    """
    Then it fail with error "Unable to find minion non-existed-minion" and code "MDD03"

  Scenario: Unregister not registered minion
    Given "dev" group
    And open alive "master" master in "dev" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'minion-fqdn',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.unregister_minion(
        i_fqdn    => 'minion-fqdn'
    )
    """

  Scenario: Failover minion
    Given "testing" group
    And open alive "master" master in "testing" group
    And open alive "master2" master in "testing" group
    And "minion" minion in "testing" group
    When I execute query
    """
    SELECT * FROM code.get_minion('minion')
    """
    Then it returns one row matches
    """
    master_fqdn: master
    """
    When I successfully execute query
    """
    SELECT * FROM code.update_master_check('master', 'manualchecker', false)
    """
    And I execute query
    """
    SELECT * FROM code.failover_minions(100)
    """
    Then it returns one row matches
    """
    fqdn: minion
    master_fqdn: master2
    """
    When I successfully execute query
    """
    SELECT * FROM code.update_master_check('master2', 'manualchecker', false)
    """
    And I execute query
    """
    SELECT * FROM code.failover_minions(100)
    """
    Then it returns nothing
    When I successfully execute query
    """
    SELECT * FROM code.update_master_check('master', 'manualchecker', true)
    """
    And I execute query
    """
    SELECT * FROM code.failover_minions(100)
    """
    Then it returns one row matches
    """
    fqdn: minion
    master_fqdn: master
    """

  Scenario: Recreate minion in upsert if enabled
    Given "dev" group
    And "another" group
    And open alive "open.master" master in "dev" group
    And open alive "another.master" master in "another" group
    And successfully executed query
    """
    SELECT * FROM code.create_minion(
        i_fqdn          => 'my.minion',
        i_deploy_group  => 'dev',
        i_auto_reassign => true
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.register_minion(
        i_fqdn    => 'my.minion',
        i_pub_key => 'some_pub_key'
    )
    """
    And successfully executed query
    """
    SELECT * FROM code.delete_minion(
        i_fqdn => 'my.minion'
    )
    """
    When I execute query
    """
    SELECT * FROM code.upsert_minion(
        i_fqdn           => 'my.minion',
        i_deploy_group   => 'another',
        i_auto_reassign  => true,
        i_allow_recreate => true
    )
    """
    Then it returns one row matches
    """
    fqdn: my.minion
    deploy_group: another
    master_fqdn: another.master
    auto_reassign: true
    pub_key: null
    registered: false
    deleted: false
    """
