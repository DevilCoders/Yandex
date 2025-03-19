# spinnaker pipelines

This project contains json files for Engineering Infra spinnaker pipelines

## Requirements:
- [cue](https://cuelang.org/docs/install/#install-cue-from-official-release-binaries)
- [spin CLI](https://spinnaker.io/docs/setup/other_config/spin/) with access to spinnaker (see: https://st.yandex-team.ru/CLOUD-93898)

## Commands
### Check pipeline schema
```shell
cue vet -c -d "#Pipeline" pipelines/*/*/*.json spinnaker-pipeline.schema.cue
# or
ya make -t
```
### Diff/export/import pipelines
```
Run "cue help commands" for more details on tasks and commands.

Usage:
  cue cmd <name> [flags]
  cue cmd <name>

Available Commands:
  export-application          Export specified application to spinnaker
  import-all-applications     Import all controlled applications with pipelines from spinnaker
  import-application          Import an application with all its pipelines
  import-project-applications Import a project with all its applications and their pipelines

Use "cue help cmd [command]" for more information about a command.

```

## Syncing spinnaker CLI
```shell
docker pull us-docker.pkg.dev/spinnaker-community/docker/spin:1.27.1
docker tag us-docker.pkg.dev/spinnaker-community/docker/spin:1.27.1 cr.yandex/yc-internal/spin:1.27.1
docker push cr.yandex/yc-internal/spin:1.27.1
```
