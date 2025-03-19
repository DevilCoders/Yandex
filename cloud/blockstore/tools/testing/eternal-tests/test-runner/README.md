# Eternal test runner

### Description
Sets up instances with attached secondary disk on a yandex cloud cluster using `ycp`, copies `eternal-load` binary to the created instance via `sftp` and runs tests from [here](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/eternal-tests/test-runner/lib/test_configs.py?#L63). `eternal-load` can be built from [sources](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/eternal-tests/eternal-load).

### Configure
1. Install `ycp` (see [instructions](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/ci/fio_performance_test_suite)).

2. Build `eternal-load`:
    ```(bash)
    $ ya make -j16 ../eternal-load
    ```

3. Build `test-runner`:
    ```(bash)
    $ ya make -j16
    ```

### Commands
- `setup-test` - setup and run new test (create instance, disk, fill disk with random data and run `eternal-load`)
- `delete-test` - delete test and instance which hosts it
- `stop-load` / `continue-load` - stop load or continue load on stopped instance 
- `rerun-load` - restarts loader from scratch and reruns load
- `add-auto-run` - add test to autorun after reboot

- `setup-db-test` - setup and run new tets with db (creates instance, disk and init db for test)
- `rerun-db-load` - rerun load for test with db

#### Required arguments
- `--test-case` - name of test to run or `all` to rerun all tests
- `--cluster` - cluster where to run test (same as for `ycp`)
- `--write-rate` (only for `rerun-load`)

#### Optional arguments
- `--refill` - one should specify it for refilling disk with random data (e.g. after disk recreation)
- `--placement-group-name` - placement group where instance will be created
- `--scp-load-binary` - one should specify it for updating `eternal-load` binary


### Examples

* Add new test
    * add test configs (`_DISK_CONFIGS/_FS_CONFIGS` and `_LOAD_CONFIGS`) [here](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/eternal-tests/test-runner/lib/test_configs.py?#L63)
    * execute
        ```(bash)
        $ ./yc-nbs-run-eternal-load-tests setup-test --cluster {cluster} --test-case {test-case}
        ```
    * update solomon dashboards (something like [this](https://solomon.yandex-team.ru/?project=nbs&cluster=yandexcloud_prod_vla&dashboard=nbs-eternal-tests-prod-overview))

* Rerun all stopped tests on cluster:
```(bash)
$ ./yc-nbs-run-eternal-load-tests rerun-load --cluster {cluster} --test-case all
```

* stop-load:
```(bash)
$ ./yc-nbs-run-eternal-load-tests stop-load --cluster {cluster} --test-case {test-case}
```

* rerun-db-load:
```(bash)
$ ./yc-nbs-run-eternal-load-tests rerun-db-load --cluster {cluster} --test-case {test_case}
```

### Other
You can create new placement group with `{name}` `{cluster}` `{folder-id}` using `ycp`:
```(bash)
$ ycp compute placement-group create -r create-placement.yaml --profile {cluster}
$ cat create-placement.yaml
description: ""
folder_id: {folder_id}
spread_placement_strategy: {
    "max_instances_per_node": "1",
    }
name: {name}
references: []

```
