# Check nrd disk emptiness test

### Description
Creates instance with attached secondary disk on a yandex cloud cluster using `ycp`. \
Copies `verify-test` binary to the created instance via `scp`. \
`verify-test` validates that created disk is empty. \
`verify-test` can be built from [sources](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/verify-test).

### Usage
Build `verify-test` binary from [sources](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/verify-test):
```(bash)
$ ya make ../../testing/verify-test
$ ln -s ../../testing/verify-test/verify-test
```

Build and launch `yc-nbs-check-emptiness-test` on `prod`:
```(bash)
$ ya make -j16
$ ./yc-nbs-ci-check-nrd-emptiness-test --cluster prod --verify-test-path ./verify-test
```
