# Dataproc Distro 
This is the fork of Apache Bigtop for building dataproc packages.

## Structure
* `packages/` - scripts for building packages
* `toolchain/` - scripts for building environment for building packages
* `output/` - directory with builded packages
* `bigtop.bom` - specification of building targets
* `build.gradle` - gradle specification
* `packages.gradle` - gradle code for building targets
* `.m2/` - generated directory for caching maven deps, reusable
* `.secrets/` - generated directory for forwarding secrets to building environment

## Things:
* [dataproc-distro-toolchain](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocDistroToolchain) - Building docker image for building packages
* [dataproc-distro-repository](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocDistroRepository) - Building new apt repository
* [lockbox secrets](https://console.cloud.yandex.ru/folders/b1gedbdjg7i4k0dmvfpf/lockbox/secret/e6q6ghlepvbtispv83a9/overview) - gpg keys and s3 credentials
* [docker registry](https://console.cloud.yandex.ru/folders/b1gedbdjg7i4k0dmvfpf/container-registry/registries/crp2tig37v7291et7rfp/overview) - registry with building environments
* [public repository](https://console.cloud.yandex.ru/folders/b1gh1aapa2udq6qobq08/storage/bucket/dataproc?key=ci%2F) - bucket with published apt repositories
* Yandex Vault [gpg private key](https://yav.yandex-team.ru/secret/sec-01e20rbvys4q52pmpqhznvgzv4/explore/versions) and [gpg public key](https://yav.yandex-team.ru/secret/sec-01e20r0az8zqqpyrn0cvf9j4c9/explore/versions)
* [Yandex vault s3 credentials](https://yav.yandex-team.ru/secret/sec-01fctbdqxr6zf7gjnhq02pabeb/explore/versions)