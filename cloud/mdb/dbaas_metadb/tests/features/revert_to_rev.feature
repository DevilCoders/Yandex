Feature: Revert to rev utility

    Background: Database at last version
        Given default database
        And cloud with default quota
        And folder

    @revert
    Scenario: Revert cluster to rev
        . Create ClickHouse cluster that contains:
        .   - ZooKeeper subcluster
        .       - with one ZK host
        .   - ClickHouse subcluster
        .       - 2 ClickHouse shard with 3 hosts each
        .   - Fill pillar:
        .       - for cluster
        .       - for each shard
        .
        . Then in one change:
        .   - delete ClickHouse hosts from second shard
        .   - delete second ClickHouse shard
        .   - update pillar for first shard
        .   - update cluster pillar
        .   - update cluster name
        .
        Given vars "cid, zk_subcid, zk_fqdn, ch_subcid"
        And vars "ch_shard_id1, ch_fqdn1_1, ch_fqdn1_2, ch_fqdn1_3"
        And vars "ch_shard_id2, ch_fqdn2_1, ch_fqdn2_2, ch_fqdn2_3"
        And "flavor1" flavor
        And successfully executed script
        """
        DO $$
        DECLARE
        BEGIN
            PERFORM code.create_cluster(
                i_cid          => :cid,
                i_name         => 'initial-cluster-name' ,
                i_type         => 'clickhouse_cluster',
                i_env          => 'qa',
                i_public_key   => '',
                i_network_id   => '',
                i_folder_id    => :folder_id,
                i_description  => NULL
            );

            -- ZooKeeper
            PERFORM code.add_subcluster(
                i_cid    => :cid,
                i_subcid => :zk_subcid,
                i_name   => 'zk-subcluster-name',
                i_roles  => CAST('{zk}' AS dbaas.role_type[]),
                i_rev    => 1
            );

            PERFORM code.add_host(
                i_subcid           => :zk_subcid,
                i_shard_id         => NULL,
                i_space_limit      => 100500,
                i_flavor_id        => :flavor_id,
                i_geo              => 'dc1',
                i_fqdn             => :zk_fqdn,
                i_disk_type        => 'disk_type1',
                i_subnet_id        => '',
                i_assign_public_ip => false,
                i_cid              => :cid,
                i_rev              => 1
            );

            -- ClickHouse
            PERFORM code.add_subcluster(
                i_cid    => :cid,
                i_subcid => :ch_subcid,
                i_name   => 'clickhouse-subcluster-name',
                i_roles  => CAST('{clickhouse_cluster}' AS dbaas.role_type[]),
                i_rev    => 1
            );

            PERFORM code.add_shard(
                i_cid      => :cid,
                i_subcid   => :ch_subcid,
                i_shard_id => shard_id,
                i_name     => shard_name,
                i_rev      => 1
            ) FROM (
                VALUES
                    (:ch_shard_id1, 'ch-shard-name-1'),
                    (:ch_shard_id2, 'ch-shard-name-2')
            ) AS t(shard_id, shard_name);

            PERFORM code.add_host(
                i_subcid           => :ch_subcid,
                i_shard_id         => shard_id,
                i_space_limit      => 100500,
                i_flavor_id        => :flavor_id,
                i_geo              => geo,
                i_fqdn             => fqdn,
                i_disk_type        => 'disk_type1',
                i_subnet_id        => '',
                i_assign_public_ip => false,
                i_cid              => :cid,
                i_rev              => 1
            ) FROM (
                VALUES
                    (:ch_shard_id1, :ch_fqdn1_1, 'dc1'),
                    (:ch_shard_id1, :ch_fqdn1_2, 'dc2'),
                    (:ch_shard_id1, :ch_fqdn1_3, 'dc3'),
                    (:ch_shard_id2, :ch_fqdn2_1, 'dc1'),
                    (:ch_shard_id2, :ch_fqdn2_2, 'dc2'),
                    (:ch_shard_id2, :ch_fqdn2_3, 'dc3')
                ) AS t (shard_id, fqdn, geo);

            -- Add pillars
            PERFORM code.add_pillar(
                i_cid   => :cid,
                i_rev   => 1,
                i_key   => key,
                i_value => CAST(value AS jsonb)
            ) FROM (
                VALUES
                    (
                        code.make_pillar_key(i_cid => :cid),
                        '{"cluster-pillar": "initialized-at-create"}'
                    ),
                    (
                        code.make_pillar_key(i_shard_id => :ch_shard_id1),
                        '{"shard-1-pillar": "initialized-at-create"}'
                    ),
                    (
                        code.make_pillar_key(i_shard_id => :ch_shard_id2),
                        '{"shard-2-pillar": "initialized-at-create"}'
                    )
            ) AS t (key, value);

            PERFORM code.complete_cluster_change(
                i_cid          => :cid,
                i_rev          => 1
            );
        END;
        $$;
        """

        # Verify what we create
        When I execute query
        """
        SELECT name,
               rev,
               to_json(labels) AS labels,
               (SELECT count(*)
                  FROM code.get_hosts_by_cid(cid)) AS hosts_count,
               (SELECT jsonb_agg(value ORDER BY priority)
                  FROM code.get_pillar_by_host(:ch_fqdn1_1)
                 WHERE priority >= 'cid') AS ch_host_pillar
          FROM code.get_clusters(
            i_folder_id => :folder_id,
            i_limit     => NULL,
            i_cid       => :cid)
        """
        Then it returns one row matches
        """
        name: initial-cluster-name
        rev: 1
        labels: []
        hosts_count: 7
        ch_host_pillar:
            - {"cluster-pillar": "initialized-at-create"}
            - {"shard-1-pillar": "initialized-at-create"}
        """

        When I successfully execute script
        """
        DO $$
        BEGIN
            PERFORM code.lock_cluster(:cid);
            PERFORM code.delete_hosts(
                i_fqdns => ARRAY[:ch_fqdn2_1, :ch_fqdn2_2, :ch_fqdn2_3],
                i_cid   => :cid,
                i_rev   => 2
            );
            PERFORM code.delete_shard(
                i_shard_id => :ch_shard_id2,
                i_cid      => :cid,
                i_rev      => 2
            );
            PERFORM code.update_pillar(
                i_cid   => :cid,
                i_rev   => 2,
                i_key   => code.make_pillar_key(i_shard_id=>:ch_shard_id1),
                i_value => CAST('{"shard-1-pillar": "updated-with-change"}' AS jsonb)
            );
            PERFORM code.update_pillar(
                i_cid   => :cid,
                i_rev   => 2,
                i_key   => code.make_pillar_key(i_cid=>:cid),
                i_value => CAST('{"cluster-pillar": "updated-with-change"}' AS jsonb)
            );
            PERFORM code.update_cluster_name(
                i_cid   => :cid,
                i_rev   => 2,
                i_name  => 'changed-cluster-name'
            );
            PERFORM code.set_labels_on_cluster(
                i_folder_id => :folder_id,
                i_cid       => :cid,
                i_labels    => ARRAY[('env', 'prod')]::code.label[],
                i_rev       => 2
            );
            PERFORM code.set_maintenance_window_settings(
                i_cid       => :cid,
                i_day       => 'MON',
                i_hour      => 1,
                i_rev       => 2
            );
            PERFORM code.complete_cluster_change(
                i_cid          => :cid,
                i_rev          => 2
            );
        END $$
        """

        # Verify what we update
        When I execute query
        """
        SELECT name,
               rev,
               to_json(labels) AS labels,
               (SELECT count(*)
                  FROM code.get_hosts_by_cid(cid)) AS hosts_count,
               (SELECT jsonb_agg(value ORDER BY priority)
                  FROM code.get_pillar_by_host(:ch_fqdn1_1)
                 WHERE priority >= 'cid') AS ch_host_pillar,
               (SELECT day
                  FROM dbaas.maintenance_window_settings
                  WHERE cid = :cid) AS maintenance_day,
               (SELECT hour
                  FROM dbaas.maintenance_window_settings
                  WHERE cid = :cid) AS maintenance_hour
          FROM code.get_clusters(
            i_folder_id => :folder_id,
            i_limit     => NULL,
            i_cid       => :cid)
        """
        Then it returns one row matches
        """
        name: changed-cluster-name
        rev: 2
        labels: [{"key":"env", "value": "prod"}]
        maintenance_day: 'MON'
        maintenance_hour: 1
        hosts_count: 4
        ch_host_pillar:
            - {"cluster-pillar": "updated-with-change"}
            - {"shard-1-pillar": "updated-with-change"}
        """

        When I execute query
        """
        SELECT code.revert_cluster_to_rev(
            i_cid    => :cid,
            i_rev    => 1,
            i_reason => 'test'
        )
        """
        Then it success

        # Verify what we restore
        When I execute query
        """
        SELECT name,
               rev,
               to_json(labels) AS labels,
               (SELECT count(*)
                  FROM code.get_hosts_by_cid(cid)) AS hosts_count,
               (SELECT jsonb_agg(value ORDER BY priority)
                  FROM code.get_pillar_by_host(:ch_fqdn1_1)
                 WHERE priority >= 'cid') AS ch_host_pillar,
               (SELECT day
                  FROM dbaas.maintenance_window_settings
                  WHERE cid = :cid) AS maintenance_day,
               (SELECT hour
                  FROM dbaas.maintenance_window_settings
                  WHERE cid = :cid) AS maintenance_hour
          FROM code.get_clusters(
            i_folder_id => :folder_id,
            i_limit     => NULL,
            i_cid       => :cid)
        """
        Then it returns one row matches
        """
        name: initial-cluster-name
        rev: 3
        labels: []
        maintenance_day: null
        maintenance_hour: null
        hosts_count: 7
        ch_host_pillar:
            - {"cluster-pillar": "initialized-at-create"}
            - {"shard-1-pillar": "initialized-at-create"}
        """


    Scenario: Revert non-existing cluster to rev
        When I execute query
        """
        SELECT code.revert_cluster_to_rev(
            i_cid    => 'non-existed-cluster-id',
            i_rev    => 1,
            i_reason => 'test'
        )
        """
        Then it fail with error "Unable to find cluster cid=non-existed-cluster-id to lock"


    Scenario: Revert cluster to non-existing rev
        Given cluster
        When I execute query
        """
        SELECT code.revert_cluster_to_rev(
            i_cid    => :cid,
            i_rev    => 100500,
            i_reason => 'test'
        )
        """
        Then it fail with error matches "Unable to find clusters_revs cid=[^\s]+, rev=100500"


    Scenario: Revert cluster to rev before move
        Given cluster
        And other folder
        And successfully executed cluster change
        """
        SELECT cid
          FROM code.update_cluster_folder(
            i_cid       => :cid,
            i_folder_id => :other_folder_id,
            i_rev       => :rev
        )
        """
        When I successfully execute query
        """
        SELECT code.revert_cluster_to_rev(
            i_cid    => :cid,
            i_rev    => 1,
            i_reason => 'test'
        )
        """
        And I execute query
        """
        SELECT rev
          FROM code.get_clusters(
            i_folder_id => :folder_id,
            i_limit     => NULL,
            i_cid       => :cid)
        """
        Then it returns one row matches
        """
        rev: 3
        """
