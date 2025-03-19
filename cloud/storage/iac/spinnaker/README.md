# spinnaker pipelines

This project contains json files for NBS spinnaker pipelines

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
Usage:
  cue cmd <name> [inputs] [flags]
  cue cmd [command]

Available Commands:
  export      Export specified pipeline file to spinnaker
  export-all  Export all pipelines to spinnaker, if dryRun is specified - only show diff
  import      Import specified pipeline from specified application
  import-all  Import all known applications pipelines from spinnaker

Flags:
  -h, --help                 help for cmd
  -t, --inject stringArray   set the value of a tagged field
  -T, --inject-vars          inject system variables in tags (default true)

Global Flags:
  -E, --all-errors   print all available errors
  -i, --ignore       proceed in the presence of errors
  -s, --simplify     simplify output
      --strict       report errors for lossy mappings
      --trace        trace computation
  -v, --verbose      print information about progress

Use "cue help cmd [command]" for more information about a command.
```

