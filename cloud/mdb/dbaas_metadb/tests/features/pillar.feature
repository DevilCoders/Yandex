Feature: Pillar helpers
  Background:
    Given default database
    And cloud with default quota
    And folder
    And "flavor1" flavor

  Scenario: Add pillar
    Given new fqdn
    And cluster
    And subcluster
    And shard
    And host
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_cid=>:cid),
        i_value => '{"from": "cluster"}'
    )
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_subcid=>:subcid),
        i_value => '{"from": "subcluster"}'
    )
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_shard_id=>:shard_id),
        i_value => '{"from": "shard"}'
    )
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_fqdn=>:fqdn),
        i_value => '{"from": "host"}'
    )
    """
    Then it success
    When I execute query
    """
    SELECT (value->>'from')::text, priority
      FROM code.get_pillar_by_host(:fqdn)
     WHERE priority IN ('cid', 'subcid', 'shard_id', 'fqdn')
     ORDER BY priority
    """
    Then it returns
    """
    - [cluster, cid]
    - [subcluster, subcid]
    - [shard, shard_id]
    - [host, fqdn]
    """

  Scenario: Add pillar writes to cluster_changes
    Given cluster
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_cid=>:cid),
        i_value => '{"it": "works!"}'
    )
    """
    Then it success
    When I execute query
    """
    SELECT jsonb_pretty(changes)::text AS changes
      FROM dbaas.clusters_changes
     WHERE cid = :cid
       AND rev = :rev
    """
    Then it returns one row matches
    """
    changes: |
        [
            {
                "add_pillar": {
                    "cid": "{{ cid }}",
                    "fqdn": null,
                    "subcid": null,
                    "shard_id": null
                }
            }
        ]
    """

  Scenario Outline: Add pillar with bad key
    Given cluster
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => <bad-key>,
        i_value => '{"from": "cluster"}'
    )
    """
    Then it fail with error matches "<message>"

  Examples:
  | bad-key                        | message                                         |
  | code.make_pillar_key()         | All pillar-components are null                  |
  | code.make_pillar_key('1', '2') | Only one pillar-key component should be defined |


  @update
  Scenario Outline: <name> pillar
    Given new fqdn
    And cluster with pillar
    """
    {"from": "xxx"}
    """
    And subcluster with pillar
    """
    {"from": "yyy"}
    """
    And shard with pillar
    """
    {"from": "zzz"}
    """
    And host with pillar
    """
    {"from": "iii"}
    """
    When I execute cluster change
    """
    SELECT * FROM <function>(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_cid=>:cid),
        i_value => '{"from": "cluster"}'
    )
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM <function>(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_subcid=>:subcid),
        i_value => '{"from": "subcluster"}'
    )
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM <function>(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_shard_id=>:shard_id),
        i_value => '{"from": "shard"}'
    )
    """
    Then it success
    When I execute cluster change
    """
    SELECT * FROM <function>(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_fqdn=>:fqdn),
        i_value => '{"from": "host"}'
    )
    """
    Then it success
    When I execute query
    """
    SELECT (value->>'from')::text, priority
      FROM code.get_pillar_by_host(:fqdn)
     WHERE priority IN ('cid', 'subcid', 'shard_id', 'fqdn')
     ORDER BY priority
    """
    Then it returns
    """
    - [cluster, cid]
    - [subcluster, subcid]
    - [shard, shard_id]
    - [host, fqdn]
    """

  Examples:
  | function           | name   |
  | code.update_pillar | Update |
  | code.upsert_pillar | Upsert |

  Scenario Outline: <name> pillar writes to cluster_changes
    Given cluster with pillar
    """
    {"old": "value"}
    """
    When I execute cluster change
    """
    SELECT * FROM code.<function>(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_cid=>:cid),
        i_value => '{"it": "works!"}'
    )
    """
    Then it success
    When I execute query
    """
    SELECT jsonb_pretty(changes)::text AS changes
      FROM dbaas.clusters_changes
     WHERE cid = :cid
       AND rev = :rev
    """
    Then it returns one row matches
    """
    changes: |
        [
            {
                "<function>": {
                    "cid": "{{ cid }}",
                    "fqdn": null,
                    "subcid": null,
                    "shard_id": null
                }
            }
        ]
    """
  Examples:
  | name    | function      |
  | Update  | update_pillar |
  | Upsert  | upsert_pillar |

  Scenario Outline: Update pillar with bad key
    Given cluster with pillar
    """
    {"foo": "bar"}
    """
    When I execute cluster change
    """
    SELECT * FROM code.update_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => <bad-key>,
        i_value => '{"from": "cluster"}'
    )
    """
    Then it fail with error matches "<message>"

  Examples:
  | bad-key                        | message                                         |
  | code.make_pillar_key()         | All pillar-components are null                  |
  | code.make_pillar_key('1', '2') | Only one pillar-key component should be defined |

  Scenario Outline: Upsert pillar with bad key
    Given cluster with pillar
    """
    {"foo": "bar"}
    """
    When I execute cluster change
    """
    SELECT * FROM code.upsert_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => <bad-key>,
        i_value => '{"from": "cluster"}'
    )
    """
    Then it fail with error matches "<message>"

  Examples:
  | bad-key                        | message                                         |
  | code.make_pillar_key()         | All pillar-components are null                  |
  | code.make_pillar_key('1', '2') | Only one pillar-key component should be defined |


  Scenario: Get pillar by host
    Given cluster type pillar
    """
    type: postgresql_cluster
    value: '{"from": "cluster_type"}'
    """
    And role pillar
    """
    type: postgresql_cluster
    role: postgresql_cluster
    value: '{"from": "role"}'
    """
    And cluster with pillar
    """
    {"from": "cluster"}
    """
    And subcluster with pillar
    """
    {"from": "subcluster"}
    """
    And host with pillar
    """
    {"from": "host"}
    """
    When I successfully execute query
    """
    UPDATE dbaas.default_pillar
    SET value = '{"from": "default_pillar"}'::jsonb
    WHERE id = 1
    """
    When I execute query
    """
    SELECT (value->>'from')::text, priority
      FROM code.get_pillar_by_host(:fqdn)
     ORDER BY priority
    """
    Then it returns
    """
    - [default_pillar, default]
    - [cluster_type, cluster_type]
    - [role, role]
    - [cluster, cid]
    - [subcluster, subcid]
    - [host, fqdn]
    """

  Scenario: Get pillar by host with target_id
    Given target_pillar id
    And cluster type pillar
    """
    type: postgresql_cluster
    value: '{"from": "cluster_type"}'
    """
    And role pillar
    """
    type: postgresql_cluster
    role: postgresql_cluster
    value: '{"from": "role"}'
    """
    And cluster with pillar
    """
    {"from": "cluster"}
    """
    And cluster target_pillar
    """
    {"from": "cluster-target"}
    """
    And subcluster with pillar
    """
    {"from": "subcluster"}
    """
    And subcluster target_pillar
    """
    {"from": "subcluster-target"}
    """
    And host with pillar
    """
    {"from": "host"}
    """
    And host target_pillar
    """
    {"from": "host-target"}
    """
    When I successfully execute query
    """
    UPDATE dbaas.default_pillar
    SET value = '{"from": "default_pillar"}'::jsonb
    WHERE id = 1
    """
    When I execute query
    """
    SELECT (value->>'from')::text, priority
      FROM code.get_pillar_by_host(:fqdn, :target_pillar_id)
     ORDER BY priority
    """
    Then it returns
    """
    - [default_pillar, default]
    - [cluster_type, cluster_type]
    - [role, role]
    - [cluster, cid]
    - [cluster-target, target_cid]
    - [subcluster, subcid]
    - [subcluster-target, target_subcid]
    - [host, fqdn]
    - [host-target, target_fqdn]
    """

  Scenario Outline: Test that can add expiration with valid value
    Given new fqdn
    And cluster
    And subcluster
    And shard
    And host
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_fqdn=>:fqdn),
        i_value => '{"cert.expiration": "<GOOD-VALUE>"}'
    )
    """
    Then it success
    Examples:
      | GOOD-VALUE                     |
      | 2022-06-04T15:22:43Z           |
      | 2022-06-04T15:22:43.0000Z      |
      | 2022-06-04T15:22:43.0000+12:34 |

  Scenario Outline: Test that can not add expiration with invalid value
    Given new fqdn
    And cluster
    And subcluster
    And shard
    And host
    When I execute cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_fqdn=>:fqdn),
        i_value => '{"cert.expiration": "<BAD-VALUE>"}'
    )
    """
    Then it fail with error "<message>"
    Examples:
        | BAD-VALUE                       | message                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
        | 2022-06-04T15:22:43.Z           | violates check constraint "check_host_cert_expiration_valid_timestamp_with_tz" |
        | 2                               | violates check constraint "check_host_cert_expiration_valid_timestamp_with_tz" |
        | 2022-06-04T15:22:43.0000+12:34Z | violates check constraint "check_host_cert_expiration_valid_timestamp_with_tz" |
