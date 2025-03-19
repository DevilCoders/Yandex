# Checkpoint validation test

### Description
Validates checkpoints on eternal verify-checkpoint test disk.

### Usage
Build `checkpoint-validator` binary from [sources](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/eternal-tests/checkpoint-validator):
```(bash)
$ ya make ../../testing/eternal-tests/checkpoint-validator
```

Build and launch `yc-nbs-ci-checkpoint-validation-test` on `hw-nbs-stable-lab`:
```(bash)
$ ya make
$ ./yc-nbs-ci-checkpoint-validation-test --cluster hw-nbs-stable-lab --validator-path {path}
```
