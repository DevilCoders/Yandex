Feature: Host helpers
  Background:
    Given default database
    And cloud with default quota
    And folder
    And cluster with pillar
    """
    {"from": "cluster"}
    """
    And subcluster with pillar
    """
    {"from": "subcluster"}
    """
    And shard

  Scenario: Add host and get it
    Given new fqdn
    And "flavor1" flavor
    And expected row "new_host"
    """
    subcid: {{ subcid }}
    shard_id: null
    space_limit: 42
    flavor_id: {{ flavor_id }}
    geo: dc1
    fqdn: {{ fqdn }}
    vtype: porto
    vtype_id: null
    roles: '{postgresql_cluster}'
    subnet_id: dummy_subnet
    assign_public_ip: true
    """
    When I execute cluster change
    """
    SELECT subcid, shard_id, space_limit, cast(flavor_id AS text),
           geo, fqdn, vtype, vtype_id, roles,
           subnet_id, assign_public_ip
      FROM code.add_host(
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_fqdn             => :fqdn,
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => true,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it returns one row matches "new_host"
    When I execute query
    """
    SELECT subcid, shard_id, space_limit, cast(flavor_id AS text),
           geo, fqdn, vtype, vtype_id, roles,
           subnet_id, assign_public_ip
      FROM code.get_host(:fqdn)
    """
    Then it returns one row matches "new_host"
    When I execute query
    """
    SELECT subcid, shard_id, space_limit, cast(flavor_id AS text),
           geo, fqdn, vtype, vtype_id, roles,
           subnet_id, assign_public_ip
      FROM code.get_hosts_by_cid(:cid)
    """
    Then it returns one row matches "new_host"

  Scenario: Add multiple hosts and get them
    Given new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And "flavor1" flavor
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn1,
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn        => :fqdn2,
        i_subcid      => :subcid,
        i_shard_id    => NULL,
        i_space_limit => 42::bigint,
        i_flavor_id   => :flavor_id,
        i_geo         => 'dc1',
        i_disk_type   => 'disk_type1',
        i_subnet_id   => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_cid(:cid)
    """
    Then it returns "2" rows matches
    """
    - subcid: {{ subcid }}
      fqdn: {{ fqdn1 }}
    - subcid: {{ subcid }}
      fqdn: {{ fqdn2 }}
    """

  Scenario: Get shard hosts
    Given new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And "flavor1" flavor
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn1,
        i_subcid           => :subcid,
        i_shard_id         => :shard_id,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn2,
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_shard(:shard_id)
    """
    Then it returns one row matches
    """
    fqdn: {{ fqdn1 }}
    subcid: {{ subcid }}
    shard_id: {{ shard_id }}
    space_limit: 42
    geo: dc1
    vtype: porto
    vtype_id: null
    subnet_id: dummy_subnet
    assign_public_ip: false
    """

  Scenario: Get shard hosts for deleting clusters
    Given "flavor1" flavor
    And new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And task_id
    And successfully executed cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn1,
        i_subcid           => :subcid,
        i_shard_id         => :shard_id,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    And successfully executed cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn2,
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    When I add "postgresql_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    When I generate new task_id
    And I add "postgresql_cluster_delete" task
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_shard(:shard_id)
    """
    Then it success
    But it returns nothing
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_shard(
          i_shard_id   => :shard_id,
          i_visibility => 'all')
    """
    Then it returns one row matches
    """
    fqdn: {{ fqdn1 }}
    subcid: {{ subcid }}
    shard_id: {{ shard_id }}
    space_limit: 42
    geo: dc1
    vtype: porto
    vtype_id: null
    subnet_id: dummy_subnet
    assign_public_ip: false
    """

  Scenario: Add host with unknown geo
    Given new fqdn
    And "flavor1" flavor
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn,
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'Mordor',
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it fail with error "Can't find geo_id for geo: Mordor"

  Scenario: Add host with unknown disk_type_id
    Given new fqdn
    And "flavor1" flavor
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_fqdn             => :fqdn,
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_disk_type        => 'floppy',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it fail with error "Can't find disk_type_id for disk_type_ext_id: floppy"

  Scenario: Update host
    Given new fqdn
    And "flavor1" flavor
    When I execute cluster change
    """
    SELECT * FROM code.add_host(
        i_subcid           => :subcid,
        i_shard_id         => NULL,
        i_space_limit      => 42,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_fqdn             => :fqdn,
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT h.*
      FROM code.update_host(
              i_fqdn        => :fqdn,
              i_space_limit => 100500,
              i_cid         => :cid,
              i_rev         => :rev) h
    """
    Then it returns one row matches
    """
    space_limit: 100500
    flavor_name: flavor1
    fqdn: {{ fqdn }}
    """
    Given "flavor4" flavor
    When I execute cluster change
    """
    SELECT h.*
      FROM code.update_host(
              i_fqdn        => :fqdn,
              i_flavor_id   => :flavor_id,
              i_cid         => :cid,
              i_rev         => :rev) h
    """
    Then it returns one row matches
    """
    space_limit: 100500
    flavor_name: flavor4
    fqdn: {{ fqdn }}
    """
    When I execute cluster change
    """
    SELECT space_limit, vtype_id::text, fqdn
      FROM code.update_host(
              i_fqdn     => :fqdn,
              i_vtype_id => 'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa',
              i_cid      => :cid,
              i_rev      => :rev)
    """
    Then it returns one row matches
    """
    space_limit: 100500
    vtype_id: 'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa'
    fqdn: {{ fqdn }}
    """
    When I execute query
    """
    SELECT h.*
      FROM code.get_host(:fqdn) h
    """
    Then it returns one row matches
    """
    space_limit: 100500
    flavor_name: flavor4
    fqdn: {{ fqdn }}
    """
    When I execute cluster change
    """
    SELECT disk_type_id, fqdn
      FROM code.update_host(
              i_fqdn      => :fqdn,
              i_disk_type => 'disk_type2',
              i_cid       => :cid,
              i_rev       => :rev)
    """
    Then it returns one row matches
    """
    fqdn: {{ fqdn }}
    disk_type_id: disk_type2
    """
    When I execute cluster change
    """
    SELECT subnet_id, fqdn
      FROM code.update_host(
              i_fqdn      => :fqdn,
              i_subnet_id => 'new_subnet',
              i_cid       => :cid,
              i_rev       => :rev)
    """
    Then it returns one row matches
    """
    fqdn: {{ fqdn }}
    subnet_id: new_subnet
    """

  @delete
  Scenario: Hosts listing for deleted clusters
    Given "flavor1" flavor
    And new fqdn
    And host
    And task_id
    And "postgresql_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_cid(:cid)
    """
    Then it returns "1" rows matches
    """
    - fqdn: {{ fqdn }}
    """
    When I generate new task_id
    And I add "postgresql_cluster_delete" task
    And I execute query
    """
    SELECT * FROM code.get_hosts_by_cid(:cid)
    """
    Then it success
    But it returns nothing
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_cid(
          i_cid        => :cid,
          i_visibility => 'all')
    """
    Then it returns "1" rows matches
    """
    - fqdn: {{ fqdn }}
    """

  @delete
  Scenario: Delete hosts
    Given "flavor1" flavor
    And new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And "fqdn1" host
    And "fqdn2" host
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_cid(:cid)
    """
    Then it returns "2" rows matches
    """
    - fqdn: {{ fqdn1 }}
    - fqdn: {{ fqdn2 }}
    """
    When I execute cluster change
    """
    SELECT *
      FROM code.delete_hosts(
        i_cid   => :cid,
        i_fqdns => ARRAY[:fqdn1],
        i_rev   => :rev
      )
    """
    Then it returns one row matches
    """
    fqdn: {{ fqdn1 }}
    """
    When I execute query
    """
    SELECT * FROM code.get_hosts_by_cid(:cid)
    """
    Then it returns one row matches
    """
    fqdn: {{ fqdn2 }}
    """

   @revs
   Scenario: get_hosts_by_cid_and_rev
    Given "flavor1" flavor
    And new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And "fqdn1" host
    And "fqdn2" host
    And actual "cid" rev is "rev_before_delete"
    When I execute cluster change
    """
    SELECT *
      FROM code.delete_hosts(
        i_cid   => :cid,
        i_fqdns => ARRAY[:fqdn1],
        i_rev   => :rev
      )
    """
    Then it success
    When I execute query
    """
    SELECT fqdn FROM code.get_hosts_by_cid_at_rev(:cid, :rev_before_delete)
    """
    Then it returns "2" rows matches
    """
    - fqdn: {{ fqdn1 }}
    - fqdn: {{ fqdn2 }}
    """