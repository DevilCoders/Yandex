# Benchmarking ClickHouse over S3

This script can be used to benchmark ClickHouse over S3 functionality. It is based on [ClickHouse performance test](https://clickhouse.tech/docs/ru/operations/performance-test/) These scripts can be used to test locally with clickhouse server installation and to test inside Cloud infrastructure.

## Test localy

Just run:
```bash
DATA_LIMIT=100 ./benchmark_s3.sh
```
## Test in Cloud infrastructure
Firstly, you need to create a `Managed ClickHouse` instance and add two ClickHouse hosts in it and tick a hybrid storage. 

Upload the current directory benchmark-clickhouse-over-s3 to the ClickHouse host:
```bash
scp -r benchmark-clickhouse-over-s3 root@some-host.db.yandex.net:/var/lib/clickhouse
```

Please note that you need to manually download the data. This is an example for 100 million rows which is default, which you need to run locally:
```bash
wget https://clickhouse-datasets.s3.yandex.net/hits/partitions/hits_100m_obfuscated_v1.tar.xz
scp hits_100m_obfuscated_v1.tar.xz root@some-host.db.yandex.net:/var/lib/clickhouse/benchmark-clickhouse-over-s3
```

On the first host you need to add data to that ClickHouse host.
```bash
cd /var/lib/clickhouse/benchmark-clickhouse-over-s3
CLOUD_RUN=true ./update_data.sh
```
Check that data is uploaded correctly. Run clickhouse-client and launch the query:
```sql
SELECT count() FROM default.hits_100m_obfuscated
```

Upload the current directory benchmark-clickhouse-over-s3 to the second ClickHouse host:
```bash
scp -r benchmark-clickhouse-over-s3 root@another-host.db.yandex.net:/var/lib/clickhouse
```
On the second host you need to run `benchmark_s3.sh`:
```bash
cd /var/lib/clickhouse/benchmark-clickhouse-over-s3
CLOUD_RUN=true DATA_LIMIT=100 CLOUD_DATA_HOST=some-host.db.yandex.net CLOUD_DATA_ADMIN_PASSWORD=some-password  ./benchmark_s3.sh  
```

## Parameters to set
You can set `STORAGE_POLICY` parameter. It is set do `"default"` by default:) You can set another one. But please make sure that you have it in your ClickHouse configuration. And make sure the server is restarted.

Here is an example of the policies config which you may add to `/etc/clickhouse-server/config.d/storage_policies.xml`. Please note that in this example `object_storage` variable identifies a disk with type S3 storage.
```xml
<policies>
    <default>
        <move_factor>0.2</move_factor>
        <volumes>
            <default>
                <disk>default</disk>
            </default>
            <object_storage>
                <disk>object_storage</disk>
                <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
            </object_storage>
        </volumes>
    </default>
    <full_s3>
        <volumes>
            <object_storage>
                <disk>object_storage</disk>
                <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
            </object_storage>
        </volumes>
    </full_s3>
    <local_only>
        <volumes>
            <default>
                <disk>default</disk>
            </default>
        </volumes>
    </local_only>
    <hybrid>
        <move_factor>0.99375</move_factor>
        <volumes>
            <default>
                <disk>default</disk>
            </default>
            <object_storage>
                <disk>object_storage</disk>
                <perform_ttl_move_on_insert>false</perform_ttl_move_on_insert>
            </object_storage>
         </volumes>
    </hybrid>
</policies>
```
`DATA_LIMIT` is a variable that says which number of rows are added to the test table during `INSERT` command. If it is not set, it is not used.

You can also get more data with `SCALE` parameter. Please check the script and [ClickHouse performance test](https://clickhouse.tech/docs/ru/operations/performance-test/)

## Comparing results
The results of the comparison are in the stdout for the script and in the file `clickhouse-benchmark-100/output.txt`

