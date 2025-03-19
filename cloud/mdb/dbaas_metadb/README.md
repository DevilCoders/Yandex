## Cluster transitions

![regenerate that image with make cluster-transitions] [trans]

[trans]: https://jing.yandex-team.ru/files/wizard/trans-v328.svg


## MetaDB schema

![regenerate that image with make metadb-schema] [meta]

[meta]: https://jing.yandex-team.ru/files/wizard/metadb-v329.svg


## Fast install `DDL` and `code`
You should:
* have `Postgres` - `brew ...`, https://postgresapp.com
* [`CREATE` expected roles](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/dbaas_metadb/run_test.sh?rev=r8164890#L22-26)

Then.

    make install-local

## Generate migration DDL
* Install [migra](https://databaseci.com/docs/migra) `pip3 install -U migra`
* Install [pglast](https://github.com/lelit/pglast) `pip3 install -U pglast`
* Install DDL from `metadb.sql`
```bash
psql postgres -c 'DROP DATABASE dbaas_metadb_one_file'
psql postgres -c 'CREATE DATABASE dbaas_metadb_one_file'
cat metadb.sql | psql dbaas_metadb_one_file
```
* Generate migration
```bash
migra  --schema=dbaas 'postgresql://localhost/dbaas_metadb' 'postgresql://localhost/dbaas_metadb_pure' | pgpp > migrations/VXXX__Your_migration_name.sql
```

## Migration tests
If you write something like. That looks like perl.

```sql
UPDATE dbaas.pillar
   SET value = new_value
  FROM (
    SELECT cid AS my_cid,
           jsonb_set(
                value,
                '{data,mysql,databases}',
                to_jsonb(
                    array(
                        SELECT jsonb_build_object(
                            dbname,
                            jsonb_build_object('lc_collate', 'latin1_swedish_ci', 'charset', 'latin1')) 
                         FROM jsonb_array_elements_text(value->'data'->'mysql'->'databases') AS dbname))) AS new_value
     FROM dbaas.pillar
    WHERE cid IN (
        SELECT cid 
          FROM dbaas.clusters
         WHERE type = 'mysql_cluster')) new_pillars
 WHERE cid = my_cid;
```
Write test for it.

### Generate data for migration
Write a scenario that fill data required for your case.

For our migration write something like

```
  @mysql
  Scenario: Setup data
    Given default database
    And cloud with default quota
    And folder
    And cluster with
      """
      type: mysql_cluster
      name: pillar test
      """
    And successfully executed cluster change
    """
    SELECT * FROM code.add_pillar(
        i_cid   => :cid,
        i_rev   => :rev,
        i_key   => code.make_pillar_key(i_cid=>:cid),
        i_value => '{"data": {"mysql": {"databases": ["foo", "bar", "baz"] }}}'
    )
    """
```

Run it.

    behave tests/features/migration_test.feature -t@mysql

You also can run all tests using
    
    ya make -tt

Then dump data.


    pg_dump dbaas_metadb --schema=dbaas --data-only --column-inserts --inserts --no-comments \
        --exclude-table-data='dbaas.flavors' \
        --exclude-table-data='dbaas.cluster_type_pillar' \
        --exclude-table-data='dbaas.default_pillar' \
        --exclude-table-data='dbaas.flavor_type' \
        --exclude-table-data='dbaas.geo' \
        --exclude-table-data='dbaas.disk_type' | rg -v -e '^(--.*|\s+|SET)' | rg -v 'SELECT pg_catalog\.(set_config|setval)' | rg -v -e '^$'


Probably you don't want include 'dictionaries'-tables in that dump. Test infrastructure should fill it for you.
Additionally you may format statements using `pgpg -c -s70` - https://pypi.org/project/pglast/


### Migration test

```
  @migration
  Scenario: Sample migration test
    Given database at "147" migration
    When I execute query
    """
       INSERT INTO dbaas.clouds VALUES (1, 'cloud1e266d46-a4af-11e9-967a-acde48001122', 48, 206158430208, 33554432, 1006632960, 40, 0, 0, 0, 0, 0, 1, 3298534883328, 0, 3298534883328, 0);
       INSERT INTO dbaas.clouds_revs VALUES (1, 1, 48, 206158430208, 33554432, 1006632960, 40, 0, 0, 0, 0, 0, 'tests.request', '2019-07-12 17:12:44.368856+03', 3298534883328, 0, 3298534883328, 0);
       INSERT INTO dbaas.folders VALUES (1, 'folder1e2973cc-a4af-11e9-91a9-acde48001122', 1);
       INSERT INTO dbaas.clusters VALUES ('1e2b3812-a4af-11e9-8391-acde48001122', 'pillar test', 'mysql_cluster', 'qa', '2019-07-12 17:12:44.39439+03', '\x', '', 1, NULL, 'CREATING', 2);
       INSERT INTO dbaas.clusters_revs VALUES ('1e2b3812-a4af-11e9-8391-acde48001122', 1, 'pillar test', '', 1, NULL, 'CREATING');
       INSERT INTO dbaas.clusters_revs VALUES ('1e2b3812-a4af-11e9-8391-acde48001122', 2, 'pillar test', '', 1, NULL, 'CREATING');
       INSERT INTO dbaas.clusters_changes VALUES ('1e2b3812-a4af-11e9-8391-acde48001122', 1, '[{"create_cluster": {}}]', '2019-07-12 17:12:44.39439+03', 'cluster.create.request');
       INSERT INTO dbaas.clusters_changes VALUES ('1e2b3812-a4af-11e9-8391-acde48001122', 2, '[{"add_pillar": {"cid": "1e2b3812-a4af-11e9-8391-acde48001122", "fqdn": null, "subcid": null, "shard_id": null}}]', '2019-07-12 17:12:44.412963+03', '');

       INSERT INTO
            dbaas.pillar
        VALUES
            ('1e2b3812-a4af-11e9-8391-acde48001122', NULL, NULL, NULL,
             '{"data": {"mysql": {"databases": ["foo", "bar", "baz"]}}}');

       INSERT INTO dbaas.pillar_revs VALUES (2, '1e2b3812-a4af-11e9-8391-acde48001122', NULL, NULL, NULL, '{"data": {"mysql": {"databases": ["foo", "bar", "baz"]}}}');
       SELECT pg_catalog.setval('dbaas.clouds_cloud_id_seq', 1, true);
       SELECT pg_catalog.setval('dbaas.folders_folder_id_seq', 1, true);
    """
    Then it success
    When I migrate database to latest migration
    Then it success
    When I execute query
    """
    SELECT jsonb_pretty(value) AS value FROM dbaas.pillar
    """
    Then it returns one row matches
    """
    value: |
        {
            "data": {
                "mysql": {
                    "databases": [
                        {
                            "foo": {
                                "charset": "latin1",
                                "lc_collate": "latin1_swedish_ci"
                            }
                        },
                        {
                            "bar": {
                                "charset": "latin1",
                                "lc_collate": "latin1_swedish_ci"
                            }
                        },
                        {
                            "baz": {
                                "charset": "latin1",
                                "lc_collate": "latin1_swedish_ci"
                            }
                        }
                    ]
                }
            }
        }
     """
```

Remove migration test when all database was migrated
