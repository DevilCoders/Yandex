Feature: Full cluster create

  Background: Default database with cloud and folder
    Given default database
    And cloud with default quota
    And folder

  Scenario: Full cluster creation in one transaction
    Given vars "cid, ch_subcid, zk_subcid, shard_id, zk_fqdn1, ch_fqdn2, ch_fqdn3"
    And "flavor1" flavor
    And new transaction
    When I execute in transaction
    """
    SELECT * FROM code.create_cluster(
        i_cid          => :cid,
        i_name         => 'cluster-name' ,
        i_type         => 'clickhouse_cluster',
        i_env          => 'qa',
        i_public_key   => '',
        i_network_id   => '',
        i_folder_id    => :folder_id,
        i_description  => NULL,
        i_x_request_id => 'cluster.create.request'
    )
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    rev: 1
    """
    When I execute in transaction
    """
    SELECT * FROM code.add_subcluster(
        i_cid    => :cid,
        i_subcid => :ch_subcid,
        i_name   => 'clickhouse-subcluster-name',
        i_roles  => cast('{clickhouse_cluster}' AS dbaas.role_type[]),
        i_rev    => 1
    )
    """
    Then it success
    When I execute in transaction
    """
    SELECT * FROM code.add_subcluster(
        i_cid    => :cid,
        i_subcid => :zk_subcid,
        i_name   => 'zk-subcluster-name',
        i_roles  => cast('{zk}' AS dbaas.role_type[]),
        i_rev    => 1
    )
    """
    Then it success
    When I execute in transaction
    """
    SELECT * FROM code.add_shard(
        i_cid      => :cid,
        i_subcid   => :ch_subcid,
        i_shard_id => :shard_id,
        i_name     => 'ch-shard-name',
        i_rev      => 1
    )
    """
    Then it success
    When I execute in transaction
    """
    SELECT * FROM code.add_host(
        i_subcid           => :zk_subcid,
        i_shard_id         => NULL,
        i_space_limit      => 100500,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_fqdn             => :zk_fqdn1,
        i_disk_type        => 'disk_type1',
        i_subnet_id        => '',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => 1
    )
    """
    Then it returns one row matches
    """
    fqdn: {{ zk_fqdn1 }}
    """
    When I execute in transaction
    """
    SELECT * FROM code.add_host(
        i_subcid           => :ch_subcid,
        i_shard_id         => :shard_id,
        i_space_limit      => 100500,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc2',
        i_fqdn             => :ch_fqdn2,
        i_disk_type        => 'disk_type1',
        i_subnet_id        => '',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => 1
    )
    """
    Then it returns one row matches
    """
    fqdn: {{ ch_fqdn2 }}
    """
    When I execute in transaction
    """
    SELECT * FROM code.add_host(
        i_subcid           => :ch_subcid,
        i_shard_id         => :shard_id,
        i_space_limit      => 100500,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc2',
        i_fqdn             => :ch_fqdn3,
        i_disk_type        => 'disk_type1',
        i_subnet_id        => '',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => 1
    )
    """
    Then it returns one row matches
    """
    fqdn: {{ ch_fqdn3 }}
    """
    When I execute in transaction
    """
    SELECT code.complete_cluster_change(
        i_cid          => :cid,
        i_rev          => 1
    )
    """
    Then it success
    When I commit transaction
    Then it success
    When I execute query
    """
    SELECT cid, rev, x_request_id, jsonb_pretty(changes) AS changes
      FROM dbaas.clusters_changes
     WHERE cid = :cid
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    rev: 1
    x_request_id: 'cluster.create.request'
    changes: |
        [
            {
                "create_cluster": {
                }
            },
            {
                "add_subcluster": {
                    "name": "clickhouse-subcluster-name",
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "subcid": "{{ ch_subcid }}"
                }
            },
            {
                "add_subcluster": {
                    "name": "zk-subcluster-name",
                    "roles": [
                        "zk"
                    ],
                    "subcid": "{{ zk_subcid }}"
                }
            },
            {
                "add_shard": {
                    "name": "ch-shard-name",
                    "subcid": "{{ ch_subcid }}",
                    "shard_id": "{{ shard_id }}"
                }
            },
            {
                "add_host": {
                    "fqdn": "{{ zk_fqdn1 }}"
                }
            },
            {
                "add_host": {
                    "fqdn": "{{ ch_fqdn2 }}"
                }
            },
            {
                "add_host": {
                    "fqdn": "{{ ch_fqdn3 }}"
                }
            }
        ]
    """
    When I execute query
    """
    SELECT *
      FROM dbaas.clusters_revs
     WHERE cid = :cid
    """
    Then it returns one row matches
    """
    cid: {{ cid }}
    rev: 1
    name: 'cluster-name'
    """
    When I execute query
    """
    SELECT *
      FROM dbaas.subclusters_revs
     WHERE cid = :cid
    """
    Then it returns "2" rows matches
    """
    - subcid: {{ ch_subcid }}
      name: 'clickhouse-subcluster-name'
      rev: 1
    - subcid: {{ zk_subcid }}
      name: 'zk-subcluster-name'
      rev: 1
    """
    When I execute query
    """
    SELECT *
      FROM dbaas.shards_revs
     WHERE subcid = :ch_subcid
    """
    Then it returns one row matches
    """
    shard_id: {{ shard_id }}
    name: 'ch-shard-name'
    rev: 1
    """
    When I execute query
    """
    SELECT *
      FROM dbaas.hosts_revs
     WHERE subcid IN (:zk_subcid, :ch_subcid)
    """
    Then it returns "3" rows matches
    """
    - fqdn: {{ zk_fqdn1 }}
      subcid: {{ zk_subcid }}
      shard_id: null
      rev: 1
    - fqdn: {{ ch_fqdn2 }}
      subcid: {{ ch_subcid }}
      shard_id: {{ shard_id }}
      rev: 1
    - fqdn: {{ ch_fqdn3 }}
      subcid: {{ ch_subcid }}
      shard_id: {{ shard_id  }}
      rev: 1
    """