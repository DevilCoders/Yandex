# Deploy packages

### Description
Updates z2 groups and launches update

### Usage
Build and launch `yc-nbs-ci-deploy-packages` on `hw-nbs-dev-lab`:
```(bash)
$ cat > packages.json <<EOF
[
    {"name": "yandex-cloud-blockstore-server", "version": "1289055342.releases.ydb.stable-22-2"},
    {"name": "yandex-cloud-storage-breakpad", "version": "1289055342.releases.ydb.stable-22-2"},
    {"name": "yc-nbs-breakpad-systemd", "version": "1289054214.trunk"},
    {"name": "yc-nbs-disk-agent-systemd", "version": "1289054214.trunk"},
    {"name": "yc-nbs-http-proxy-systemd", "version": "1289054214.trunk"},
    {"name": "yc-nbs-systemd", "version": "1289054214.trunk"},
    {"name": "yandex-search-kikimr-nbs-conf-hw-nbs-stable-lab-global", "version": "9391052.trunk.1289054214"},
    {"name": "yandex-search-kikimr-nbs-control-conf-hw-nbs-stable-lab-global", "version": "9391052.trunk.1289054214"}
]
EOF
$ ya make
$ ./yc-nbs-ci-deploy-packages -c hw-nbs-stable-lab -s blockstore -p packages.json
```

### Deploy
Build and upload `yc-nbs-ci-tools`
[docker image](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/blockstore/packages/yc-nbs-ci-tools)
via `ya package` to deploy your code on teamcity agents.
