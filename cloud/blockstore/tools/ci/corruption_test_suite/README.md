# Corruption test

### Description
Creates instance with attached secondary disk on a yandex cloud cluster using `ycp`.
Copies `verify-test` binary to the created instance via `scp`.
`verify-test` can be built from [sources](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/verify-test).

### Usage
Install `ycp` and reload shell:
```(bash)
$ curl https://s3.mds.yandex.net/mcdev/ycp/install.sh | bash
$ cat > ~/.bashrc <<EOF
PATH="$HOME/ycp/bin/:$PATH"
EOF
```

Install `ycp` config for tests:
```(bash)
$ mkdir -p ~/.config/ycp/
$ mv ~/.config/ycp/config.yaml ~/.config/ycp/config.yaml.bak
$ cp ../../../packages/yc-nbs-ci-tools/ycp-config.yaml ~/.config/ycp/config.yaml
```

Build `verify-test` binary from [sources](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/tools/testing/verify-test):
```(bash)
$ ya make ../../testing/verify-test
$ ln -s ../../testing/verify-test/verify-test
```

Build and launch `yc-nbs-ci-corruption-test` on `prod`:
```(bash)
$ ya make -j16
$ ./yc-nbs-ci-corruption-test -c prod --test-case bs512bytes-iodepth64 --verify-test-path ./verify-test
```

Patch `/etc/hosts` to be able to use `ycp` with `hw-nbs-stable-lab`:
```(bash)
$ export HW_NBS_STABLE_LAB_SEED_IP=$(host $(pssh list C@cloud_hw-nbs-stable-lab_seed) | awk '{print $5}')
$ echo -e "\n# ycp hack for hw-nbs-stable-lab\n$HW_NBS_STABLE_LAB_SEED_IP local-lb.cloud-lab.yandex.net" | sudo tee -a /etc/hosts
```

Create `/etc/ssl/certs/hw-nbs-stable-lab.pem` with the content https://paste.yandex-team.ru/3966169

Launch `yc-nbs-ci-corruption-test` on `hw-nbs-stable-lab` with `intersect-ranges` test suite:
```(bash)
$ ./yc-nbs-ci-corruption-test -c hw-nbs-stable-lab --test-case intersect-ranges --verify-test-path ./verify-test
```

### Deploy
Build and upload `yc-nbs-ci-tools`
[docker image](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/packages/yc-nbs-ci-tools)
via `ya package` to make your code be executed on teamcity agents
