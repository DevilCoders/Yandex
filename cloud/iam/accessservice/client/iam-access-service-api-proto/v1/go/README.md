# Overview

In this repository we store a module with go protobuf models and low-level grpc client code,
which are generated manually using the `protoc` library.

Our higher-level [client library for go](https://bb.yandex-team.ru/projects/CLOUD/repos/iam-access-service-client-go)
adds this module as a dependency.



# Generating go code

Our build script was heavily inspired by the
[proto generation code](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/gen_proto.mk)
from `cloud-go`.

To generate updated code use the following command:

```bash
make -f code_generator/Makefile.mk
```



# CI

Every PR and commit to `master`/`release-*` branches triggers Teamcity build which runs these simple checks:
```bash
go build ./yandex/... # just in case
go test ./yandex/...
```



# New library versions

To make a new version available for other go projects you need to run the `build` task in Teamcity
(see details [here](../README.md)).

The task pushes a new version tag into the repository which makes it available for usage from other go projects.

For debug purposes the following commands can be used:
```bash
# publishing a new version tag
git tag v1.0.0
git push origin v1.0.0

# listing published versions
git ls-remote --tags origin

# deleting an unused tag
# WARN: don't delete published (and ever used anywhere) versions!
git push -d origin v1.0.0 
```
