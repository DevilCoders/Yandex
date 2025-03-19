@migration
Feature: Migrating to v0397 version is successed

    Scenario: Migrating to v0397 version for cluster without maintenance task will update data correctly
    Given database at "396" migration
    When I execute query
    """
        INSERT INTO dbaas.clouds VALUES (1, 'cloud1e266d46-a4af-11e9-967a-acde48001122', 48, 206158430208, 33554432, 48, 40, 0, 1, 0, 0, 1, 1, 0, 0);
        INSERT INTO dbaas.clouds_revs VALUES (1, 1, 48, 206158430208, 33554432, 48, 40, 0, 'tests.request', '2019-07-12 17:12:44.368856+03', 0, 0, 1, 1, 0, 0);
        INSERT INTO dbaas.folders VALUES (1, 'folder1e2973cc-a4af-11e9-91a9-acde48001122', 1);
        INSERT INTO dbaas.clusters VALUES ('cluster_id1', 'pillar test', 'clickhouse_cluster', 'qa', '2019-07-12 17:12:44.39439+03', '\x', '', 1, NULL, 'CREATING', 2, 2);
        INSERT INTO dbaas.clusters_revs VALUES ('cluster_id1', 1, 'pillar test', '', 1, NULL, 'CREATING', NULL, NULL);
        INSERT INTO dbaas.clusters_revs VALUES ('cluster_id1', 2, 'pillar test', '', 1, NULL, 'CREATING', NULL, NULL);
        INSERT INTO dbaas.clusters_changes VALUES ('cluster_id1', 1, '[{"create_cluster": {}}]', '2019-07-12 17:12:44.39439+03', 'cluster.create.request');
        INSERT INTO dbaas.clusters_changes VALUES ('cluster_id1', 2, '[{"add_pillar": {"cid": "cluster_id1", "fqdn": null, "subcid": "subcluster_id1", "shard_id": null}}]', '2019-07-12 17:12:44.412963+03', '');
        INSERT INTO dbaas.subclusters VALUES ('subcluster_id1', 'cluster_id1', 'clickhouse_subcluster', '{clickhouse_cluster}', '2019-07-12 17:12:44.412963+03');
        INSERT INTO dbaas.subclusters_revs VALUES ('subcluster_id1', 1, 'cluster_id1', 'clickhouse_subcluster', '{clickhouse_cluster}', '2019-07-12 17:12:44.412963+03');

        INSERT INTO dbaas.pillar VALUES (NULL, 'subcluster_id1', NULL, NULL, '{"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"]}}}');
        INSERT INTO dbaas.pillar_revs VALUES (1, NULL, 'subcluster_id1', NULL, NULL, '{"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"]}}}');
    """
    Then it success
    When I migrate database to latest migration
    Then it success
    When I execute query
    """
    SELECT value
    FROM dbaas.pillar
    WHERE subcid = 'subcluster_id1'
    """
    Then it returns one row matches
    """
    value: {"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"], "keeper_hosts": {"zk1": 1, "zk2": 2, "zk3": 3}}}}
    """
    When I execute query
    """
    SELECT value
    FROM dbaas.pillar_revs
    WHERE subcid = 'subcluster_id1'
    """
    Then it returns "2" rows matches
    """
    - value: {"data": {"clickhouse": {"zk_hosts": ["zk1" ,"zk3", "zk2"]}}}
    - value: {"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"], "keeper_hosts": {"zk1": 1, "zk2": 2, "zk3": 3}}}}
    """
    When I execute query
    """
    SELECT actual_rev, next_rev
    FROM dbaas.clusters
    WHERE cid = 'cluster_id1'
    """
    Then it returns one row matches
    """
    actual_rev: 3
    next_rev: 3
    """

    Scenario: Migrating to v0397 version for cluster with maintenance task will update data correctly
    Given database at "394" migration
    When I execute query
    """
        INSERT INTO dbaas.clouds VALUES (1, 'cloud1e266d46-a4af-11e9-967a-acde48001122', 48, 206158430208, 33554432, 48, 40, 0, 1, 0, 0, 1, 1, 0, 0);
        INSERT INTO dbaas.clouds_revs VALUES (1, 1, 48, 206158430208, 33554432, 48, 40, 0, 'tests.request', '2019-07-12 17:12:44.368856+03', 0, 0, 1, 1, 0, 0);
        INSERT INTO dbaas.folders VALUES (1, 'folder1e2973cc-a4af-11e9-91a9-acde48001122', 1);
        INSERT INTO dbaas.clusters VALUES ('cluster_id1', 'pillar test', 'clickhouse_cluster', 'qa', '2019-07-12 17:12:44.39439+03', '\x', '', 1, NULL, 'CREATING', 2, 3);
        INSERT INTO dbaas.clusters_revs VALUES ('cluster_id1', 1, 'pillar test', '', 1, NULL, 'CREATING', NULL, NULL);
        INSERT INTO dbaas.clusters_revs VALUES ('cluster_id1', 2, 'pillar test', '', 1, NULL, 'CREATING', NULL, NULL);
        INSERT INTO dbaas.clusters_changes VALUES ('cluster_id1', 1, '[{"create_cluster": {}}]', '2019-07-12 17:12:44.39439+03', 'cluster.create.request');
        INSERT INTO dbaas.clusters_changes VALUES ('cluster_id1', 2, '[{"add_pillar": {"cid": "cluster_id1", "fqdn": null, "subcid": "subcluster_id1", "shard_id": null}}]', '2019-07-12 17:12:44.412963+03', '');
        INSERT INTO dbaas.subclusters VALUES ('subcluster_id1', 'cluster_id1', 'clickhouse_subcluster', '{clickhouse_cluster}', '2019-07-12 17:12:44.412963+03');
        INSERT INTO dbaas.subclusters_revs VALUES ('subcluster_id1', 1, 'cluster_id1', 'clickhouse_subcluster', '{clickhouse_cluster}', '2019-07-12 17:12:44.412963+03');

        INSERT INTO dbaas.maintenance_tasks VALUES ('cluster_id1', '', 'task_id1', '2019-07-12 17:12:44.39439+03', '2019-09-12 17:12:44.39439+03', 'PLANNED', NULL, '2019-12-12 17:12:44.39439+03');
        INSERT INTO dbaas.worker_queue VALUES ('task_id1', 'cluster_id1', '2019-07-12 17:12:44.39439+03', '2019-07-12 17:12:44.39439+03', '2019-07-12 17:12:44.39439+03', 'worker_id1', 'task_type1', '{}', true, '{}', '', 'user1', 1, 'operation_type1', '{}', FALSE, 1, NULL, NULL, NULL, NULL, '10 minutes', 0, 0, 0, FALSE, NULL, 0, NULL, 0, 0, NULL, FALSE);

        INSERT INTO dbaas.pillar VALUES (NULL, 'subcluster_id1', NULL, NULL, '{"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"]}}}');
        INSERT INTO dbaas.pillar_revs VALUES (1, NULL, 'subcluster_id1', NULL, NULL, '{"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"]}}}');
    """
    Then it success
    When I migrate database to latest migration
    Then it success
    When I execute query
    """
    SELECT value
    FROM dbaas.pillar
    WHERE subcid = 'subcluster_id1'
    """
    Then it returns one row matches
    """
    value: {"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"], "keeper_hosts": {"zk1": 1, "zk2": 2, "zk3": 3}}}}
    """
    When I execute query
    """
    SELECT value
    FROM dbaas.pillar_revs
    WHERE subcid = 'subcluster_id1'
    """
    Then it returns "2" rows matches
    """
    - value: {"data": {"clickhouse": {"zk_hosts": ["zk1" ,"zk3", "zk2"]}}}
    - value: {"data": {"clickhouse": {"zk_hosts": ["zk1", "zk3", "zk2"], "keeper_hosts": {"zk1": 1, "zk2": 2, "zk3": 3}}}}
    """
    When I execute query
    """
    SELECT actual_rev, next_rev
    FROM dbaas.clusters
    WHERE cid = 'cluster_id1'
    """
    Then it returns one row matches
    """
    actual_rev: 4
    next_rev: 4
    """
    When I execute query
    """
    SELECT status
    FROM dbaas.maintenance_tasks
    WHERE cid = 'cluster_id1'
    """
    Then it returns one row matches
    """
    status: 'CANCELED'
    """
    When I execute query
    """
    SELECT result, finish_rev
    FROM dbaas.worker_queue
    WHERE cid = 'cluster_id1'
    ORDER BY create_ts DESC
    LIMIT 1
    """
    Then it returns one row matches
    """
    result: true
    finish_rev: 4
    """