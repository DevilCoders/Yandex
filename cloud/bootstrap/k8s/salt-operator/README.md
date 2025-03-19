### kubectl plugin
#### Install
1. Install [krew](https://krew.sigs.k8s.io/docs/user-guide/setup/install/)
2. Install plugin
```shell
kubectl krew index add bootstrap https://git@git.arc-vcs.yandex-team.ru/bootstrap-krew-index
kubectl krew install bootstrap/saltformula
```
#### Releasing plugin to krew
1. Increment semver in [cmd/kubectl-saltformula/pkg.json](cmd/kubectl-saltformula/pkg.json)
2. Build and upload binaries
```shell
make release-plugin
```
2. Commit changes in cloud/bootstrap/k8s/krew-index

#### Releasing plugin to debian repo
```shell
# Only runs on linux machine with dch installed
ya package cmd/kubectl-saltformula/pkg.json
```

### Build

***
Local machine:

`ya package --target-platform linux pkg.json --docker --docker-registry=cr.yandex --docker-repository=crp136vhf7jvu9cnpvga`

`cd chart; make helm-release`


***
AW:

https://teamcity.aw.cloud.yandex.net/buildConfiguration/Selfhost_SaltOperator_DockerRelease

### Regenerating CRD's for helm chart

You need go 1.17 for generating and working with kubebuilder, because kubebuilder [only supports go 1.7 for now](https://book.kubebuilder.io/quick-start.html#prerequisites) and arcadia has go 1.18+
Example for Mac OS X:
```shell
brew install go@1.17
export GOBIN=/usr/local/Cellar/go@1.17/1.17.10 # Or your GOBIN from brew
make helm-generate
```
