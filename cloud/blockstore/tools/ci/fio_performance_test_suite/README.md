# fio performance test

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

Patch `/etc/hosts` to be able to use `ycp` with `hw-nbs-stable-lab`:
```(bash)
$ export HW_NBS_STABLE_LAB_SEED_IP=$(host $(pssh list C@cloud_hw-nbs-stable-lab_seed) | awk '{print $5}')
$ echo -e "\n# ycp hack for hw-nbs-stable-lab\n$HW_NBS_STABLE_LAB_SEED_IP local-lb.cloud-lab.yandex.net" | sudo tee -a /etc/hosts
```

Create `/etc/ssl/certs/hw-nbs-stable-lab.pem` with the content https://paste.yandex-team.ru/3966169

Launch `yc-nbs-fio-performance-test-suite` on `hw-nbs-stable-lab` with `default` test suite:
```(bash)
$ ./yc-nbs-ci-fio-performance-test-suite -c hw-nbs-stable-lab --test-suite default --no-yt --compute-node xxx-xx-x.cloud.yandex.net
```
