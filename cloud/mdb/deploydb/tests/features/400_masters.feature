Feature: Masters API

  Background: Database at last migration
    Given database at last migration

  Scenario: Create master and get it
    Given "prod" group
    When I execute query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'master.org',
        i_deploy_group => 'prod',
        i_is_open      => true,
        i_description  => 'First master'
    )
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: false
    description: 'First master'
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_master('master.org')
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: false
    description: 'First master'
    aliases: []
    """


  Scenario: Create 2 master in one group
    Given "prod" group
    When I execute query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'first.master',
        i_deploy_group => 'prod',
        i_is_open      => true,
        i_description  => 'First master'
    )
    """
    Then it returns one row matches
    """
    fqdn: 'first.master'
    deploy_group: 'prod'
    is_open: true
    description: 'First master'
    aliases: []
    """
    When I execute query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'second.master',
        i_deploy_group => 'prod',
        i_is_open      => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'second.master'
    deploy_group: 'prod'
    is_open: false
    description: null
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_masters(
        i_limit => 100,
        i_last_master_id => NULL
    )
    """
    Then it returns "2" rows matches
    """
    - id: 1
      fqdn: 'first.master'
      deploy_group: 'prod'
      is_open: true
      description: 'First master'
      aliases: []
    - id: 2
      fqdn: 'second.master'
      deploy_group: 'prod'
      is_open: false
      description: null
      aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_masters(
        i_limit => 1,
        i_last_master_id => NULL
    )
    """
    Then it returns "1" rows matches
    """
    - id: 1
      fqdn: 'first.master'
      deploy_group: 'prod'
      is_open: true
      description: 'First master'
      aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_masters(
        i_limit => 1,
        i_last_master_id => 1
    )
    """
    Then it returns "1" rows matches
    """
    - id: 2
      fqdn: 'second.master'
      deploy_group: 'prod'
      is_open: false
      description: null
      aliases: []
    """
    When I execute query
    """
    SELECT
        change_type,
        old_row,
        old_row->'fqdn' AS old_row_fqdn,
        old_row->'is_open' AS old_row_is_open,
        old_row->'description' AS old_row_description,
        new_row,
        new_row->'fqdn' AS new_row_fqdn,
        new_row->'is_open' AS new_row_is_open,
        new_row->'description' AS new_row_description
      FROM deploy.masters_change_log
    """
    Then it returns "2" rows matches
    """
    - change_type: create
      old_row: null
      new_row_fqdn: first.master
      new_row_is_open: true
      new_row_description: First master
    - change_type: create
      old_row: null
      new_row_fqdn: second.master
      new_row_is_open: false
      new_row_description: null
    """


  Scenario: Create master in non-existent group
    When I execute query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'bad.master',
        i_deploy_group => 'not-exists',
        i_is_open      => true
    )
    """
    Then it fail with error "Unable to find group with name not-exists" and code "MDD03"

  Scenario: Create master with used fqdn
    Given "testing" group
    And successfully executed query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'new.master',
        i_deploy_group => 'testing',
        i_is_open      => true
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'new.master',
        i_deploy_group => 'testing',
        i_is_open      => true
    )
    """
    Then it fail with error "duplicate key value violates unique constraint"

  Scenario: Upsert created master
    Given "testing" group
    And "prod" group
    And successfully executed query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'new.master',
        i_deploy_group => 'testing',
        i_is_open      => true
    )
    """
    When I execute query
    """
    SELECT *
      FROM code.upsert_master(
        i_fqdn         => 'new.master',
        i_deploy_group => 'prod',
        i_is_open      => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'new.master'
    deploy_group: 'prod'
    is_open: false
    is_alive: false
    aliases: []
    """

  Scenario: Create master with upsert and update it
    Given "testing" group
    And "prod" group
    When I execute query
    """
    SELECT *
      FROM code.upsert_master(
        i_fqdn         => 'new.master',
        i_deploy_group => 'testing',
        i_is_open      => true
    )
    """
    Then it returns one row matches
    """
    fqdn: 'new.master'
    deploy_group: 'testing'
    is_open: true
    is_alive: false
    aliases: []
    """
    When I execute query
    """
    SELECT *
      FROM code.upsert_master(
        i_fqdn         => 'new.master',
        i_deploy_group => 'prod',
        i_is_open      => false
    )
    """
    Then it returns one row matches
    """
    fqdn: 'new.master'
    deploy_group: 'prod'
    is_open: false
    is_alive: false
    aliases: []
    """

  Scenario: Create master and mark it alive then dead
    Given "prod" group
    When I execute query
    """
    SELECT *
      FROM code.create_master(
        i_fqdn         => 'master.org',
        i_deploy_group => 'prod',
        i_is_open      => true
    )
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: false
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_master('master.org')
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: false
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.update_master_check('master.org', 'checker', true)
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: true
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_master('master.org')
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: true
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.update_master_check('master.org', 'checker', false)
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: false
    aliases: []
    """
    When I execute query
    """
    SELECT * FROM code.get_master('master.org')
    """
    Then it returns one row matches
    """
    fqdn: 'master.org'
    deploy_group: 'prod'
    is_open: true
    is_alive: false
    aliases: []
    """


  Scenario: Quorum alive masters
    Given "testing" group
    And open alive "master" master in "testing" group
    And open alive "master2" master in "testing" group
    When I execute query
    """
    SELECT * FROM code.update_master_check('master', 'manualchecker', false)
    """
    Then it returns one row matches
    """
    fqdn: master
    is_alive: false
    """
    When I execute query
    """
    SELECT * FROM code.get_masters(i_limit => 100, i_last_master_id => NULL)
    """
    Then it returns "2" rows matches
    """
    - fqdn: master
      is_alive: false
    - fqdn: master2
      is_alive: true
    """
    When I execute query
    """
    SELECT * FROM deploy.masters_check_view
    """
    Then it returns "3" rows matches
    """
    - master_id: 1
      checker_fqdn: autochecker
      is_alive: true
    - master_id: 1
      checker_fqdn: manualchecker
      is_alive: false
    - master_id: 2
      checker_fqdn: autochecker
      is_alive: true
    """
    When I execute query
    """
    SELECT * FROM code.update_master_check('master2', 'manualchecker', false)
    """
    Then it returns one row matches
    """
    fqdn: master2
    is_alive: false
    """
    When I execute query
    """
    SELECT * FROM code.get_masters(i_limit => 100, i_last_master_id => NULL)
    """
    Then it returns "2" rows matches
    """
    - fqdn: master
      is_alive: false
    - fqdn: master2
      is_alive: false
    """
    When I execute query
    """
    SELECT * FROM deploy.masters_check_view
    """
    Then it returns "4" rows matches
    """
    - master_id: 1
      checker_fqdn: autochecker
      is_alive: true
    - master_id: 1
      checker_fqdn: manualchecker
      is_alive: false
    - master_id: 2
      checker_fqdn: autochecker
      is_alive: true
    - master_id: 2
      checker_fqdn: manualchecker
      is_alive: false
    """
    When I execute query
    """
    SELECT * FROM code.update_master_check('master', 'manualchecker', true)
    """
    Then it returns one row matches
    """
    fqdn: master
    is_alive: true
    """
    When I execute query
    """
    SELECT * FROM code.get_masters(i_limit => 100, i_last_master_id => NULL)
    """
    Then it returns "2" rows matches
    """
    - fqdn: master
      is_alive: true
    - fqdn: master2
      is_alive: false
    """
    When I execute query
    """
    SELECT * FROM deploy.masters_check_view
    """
    Then it returns "4" rows matches
    """
    - master_id: 1
      checker_fqdn: autochecker
      is_alive: true
    - master_id: 1
      checker_fqdn: manualchecker
      is_alive: true
    - master_id: 2
      checker_fqdn: autochecker
      is_alive: true
    - master_id: 2
      checker_fqdn: manualchecker
      is_alive: false
    """
