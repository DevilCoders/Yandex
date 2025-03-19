Feature: Placement_group_and_disk
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
    And new fqdn as "fqdn1"
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
  Scenario: Add host and add placement_group and add_disk then update them
    When I execute cluster change
    """
    SELECT * FROM code.create_disk_placement_group(
        i_cid              => :cid,
        i_local_id         => 1,
        i_rev              => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.create_disk_placement_group(
        i_cid              => :cid,
        i_local_id         => 0,
        i_rev              => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT cid, local_id FROM dbaas.disk_placement_groups WHERE cid = :cid
    """
    Then it returns "2" rows matches
    """
    - cid: {{ cid }}
      local_id: 1
    - cid: {{ cid }}
      local_id: 0
    """
    When I execute cluster change
    """
    SELECT * FROM code.update_disk_placement_group(
        i_cid                     => :cid,
        i_disk_placement_group_id => 'dpg1',
        i_local_id                => 0,
        i_status                  => 'COMPLETE',
        i_rev                     => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.update_disk_placement_group(
        i_cid                     => :cid,
        i_disk_placement_group_id => 'dpg2',
        i_local_id                => 1,
        i_status                  => 'COMPLETE',
        i_rev                     => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT cid, local_id, disk_placement_group_id, status FROM dbaas.disk_placement_groups WHERE cid = :cid
    """
    Then it returns "2" rows matches
    """
    - cid: {{ cid }}
      local_id: 1
      disk_placement_group_id: dpg2
      status: COMPLETE
    - cid: {{ cid }}
      local_id: 0
      disk_placement_group_id: dpg1
      status: COMPLETE
    """
    When I execute cluster change
    """
    SELECT * FROM code.create_disk(
        i_cid              => :cid,
        i_local_id         => 1,
        i_fqdn             => :fqdn1,
        i_mount_point      => 'mp1',
        i_rev              => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.create_disk(
        i_cid              => :cid,
        i_local_id         => 0,
        i_fqdn             => :fqdn1,
        i_mount_point      => 'mp2',
        i_rev              => :rev)
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.create_disk(
        i_cid              => :cid,
        i_local_id         => 1,
        i_fqdn             => :fqdn2,
        i_mount_point      => 'mp1',
        i_rev              => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT cid, fqdn, mount_point FROM dbaas.disks WHERE cid = :cid
    """
    Then it returns "3" rows matches
    """
    - cid: {{ cid }}
      fqdn: {{ fqdn1 }}
      mount_point: 'mp1'
    - cid: {{ cid }}
      fqdn: {{ fqdn1 }}
      mount_point: 'mp2'
    - cid: {{ cid }}
      fqdn: {{ fqdn2 }}
      mount_point: 'mp1'
    """
   When I execute cluster change
    """
    SELECT * FROM code.update_disk(
        i_cid           => :cid,
        i_local_id     => 1,
        i_fqdn         => :fqdn1,
        i_mount_point  => 'mp1',
        i_rev          => :rev,
        i_status       => 'COMPLETE',
        i_disk_id      => 'disk77',
        i_host_disk_id => 'mnt')
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.update_disk(
        i_cid          => :cid,
        i_local_id     => 0,
        i_fqdn         => :fqdn1,
        i_mount_point  => 'mp2',
        i_rev          => :rev,
        i_status       => 'COMPLETE',
        i_disk_id      => 'disk14',
        i_host_disk_id => 'mnt')
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.update_disk(
        i_cid          => :cid,
        i_local_id     => 1,
        i_fqdn         => :fqdn2,
        i_mount_point  => 'mp1',
        i_rev          => :rev,
        i_status       => 'COMPLETE',
        i_disk_id      => 'disk22',
        i_host_disk_id => 'mnt')
    """
    Then it success
    When I execute query
    """
    SELECT cid, fqdn, mount_point, disk_id, status FROM dbaas.disks WHERE cid = :cid
    """
    Then it returns "3" rows matches
    """
    - cid: {{ cid }}
      fqdn: {{ fqdn1 }}
      mount_point: 'mp1'
      status: COMPLETE
      disk_id: disk77
    - cid: {{ cid }}
      fqdn: {{ fqdn1 }}
      mount_point: 'mp2'
      status: COMPLETE
      disk_id: disk14
    - cid: {{ cid }}
      fqdn: {{ fqdn2 }}
      mount_point: 'mp1'
      status: COMPLETE
      disk_id: disk22
    """
    When I execute cluster change
    """
    SELECT * FROM code.create_placement_group(
        i_cid              => :cid,
        i_rev              => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT cid FROM dbaas.placement_groups WHERE cid = :cid
    """
    Then it returns "1" rows matches
    """
    - cid: {{ cid }}
    """
    When I execute cluster change
    """
    SELECT * FROM code.update_placement_group(
        i_cid                     => :cid,
        i_placement_group_id      => 'pg1',
        i_status                  => 'COMPLETE',
        i_rev                     => :rev)
    """
    Then it success
    When I execute query
    """
    SELECT cid, placement_group_id, status FROM dbaas.placement_groups WHERE cid = :cid
    """
    Then it returns "1" rows matches
    """
    - cid: {{ cid }}
      placement_group_id: pg1
      status: COMPLETE
    """
