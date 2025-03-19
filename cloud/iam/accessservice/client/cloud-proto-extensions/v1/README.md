# Overview

A copy of validation & sensitive proto extensions from cloud-go
available for repositories outside the go mono-repository.



# Development

After modifying the `.proto` sources, don't forget to re-generate and commit the go files:
```bash
make -f go/code_generator/Makefile
```



# CI

There are the following Teamcity build configurations for this repository:

## Java & Go

Verification & deployment CI tasks are [here](https://teamcity.yandex-team.ru/project/Cloud_CloudProtoExtensions?projectTab=overview).

## Python

- ~~verify python proto extensions~~ - no verification checks at this moment
- [publish python proto extensions](https://teamcity.aw.cloud.yandex.net/buildConfiguration/IAM_PublishIamAccessServicePythonProtoExtensions) -
  automatically publishes a new version of generated code



# Arcadia Sync

To sync the Bitbucket repository code with Arcadia use the following command:
```bash
./tools/arcadia_sync.sh ${PATH_TO_ARCADIA_ROOT:-$A} v1.x.y-z
```

To get available versions use the command:
```bash
git ls-remote --tags origin
```
(or check them in the [changelog](./CHANGELOG.md))
