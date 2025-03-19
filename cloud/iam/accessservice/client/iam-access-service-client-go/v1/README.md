# Overview

The go client for the IAM Access Service.

Implements the Access Service
[gRPC API specification](https://bb.yandex-team.ru/projects/CLOUD/repos/iam-access-service-api-proto).



# How to import

To import the latest client to your project use this command:
```bash
go get bb.yandex-team.ru/cloud/iam-access-service-client-go@latest
```

To import a specific version use a git tag (e.g. `v1.2.3-<suffix>`) instead of 'latest':
```bash
go get bb.yandex-team.ru/cloud/iam-access-service-client-go@<git_tag>
```
The tags are equal to versions listed in the [changelog](./CHANGELOG.md).



# Local development

You can find detailed instructions on how to set up your environment
[here](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/README.md).

### Some important notes for local development:

- add the following alias to your local `.gitconfig`:
    ```text
    [url "ssh://git@bb.yandex-team.ru/"]
      insteadOf = https://bb.yandex-team.ru/scm/
    ```
- make sure you mark the `bb.yandex-team.ru` repository as private; this will make go tools disable proxies for it:
    ```text
    go env -w GOPRIVATE=bb.yandex-team.ru
    ```

### Compilation & Testing

To check your code:
```bash
go build ./...
go test ./...
```

### Dependencies

To add, upgrade or downgrade dependencies use:
```bash
go get package.com/x/y@version
```

To clean up any unused dependencies use:
```bash
go mod tidy
```



# CI

There are the following Teamcity
[build configurations](https://teamcity.yandex-team.ru/project/Cloud_IamAccessServiceClientGo?mode=builds)
for this module:
- [build](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_IamAccessServiceClientGo_Build) -
  is run on every Bitbucket PR and commit into the `master`/`release-*` branches
- [deploy](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_IamAccessServiceClientGo_Deploy) -
  at this moment is run manually to publish a new version tag of the library



# Developing new versions

1. Make changes to the code.
2. Check the build locally (see the sections above).
3. Create a PR.
4. After PR is merged run the `deploy` task to publish the changes.
5. Update the [changelog](./CHANGELOG.md). Could be done before step 3 if you can predict the new version number.
