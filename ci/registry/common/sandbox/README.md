# Ci config
Create token by [CI tutorial](https://docs.yandex-team.ru/ci/quick-start-guide)

# Description
Use common/sandbox/deploy_binary_task in ci flow to build resource binary tasks.
There is three four jobs: build and three release. 'Build' job execute DEPLOY_BINARY_TASK and creates resource with binary.
Releases are optional, if you don't need some release type you must remove release blocks with it type from releases and flow.

By default arcadia url looks at `arcadia-arc:/#${context.target_revision.hash}`, you can redefine it by adding 'arcadia_url' parameter:
```yaml
arcadia_url: <Your url>
```
For example, if you need last trunk version, use:
```yaml
arcadia_url: arcadia-arc:/#trunk
```

By default building target is a current directory with 'a.yaml', you can redefine it by adding 'target' parameter:
```yaml
target: <Your target>
```

You can redefine 'attrs' parameter if you need special attributes for created resource.
Popular case is to set attribute task_type that used in LastBinaryTaskRelease in setting tasks resource.
Example:
```yaml
attrs:
  sync_upload_to_mds: true
  task_type: <task type in binary>
```

To speed up building you can use yt cache by setting parameter:
```yaml
use_yt_cache: true
yt_token_vault: <Your name>
```
If use_yt_cache is true, it must be secret in sandbox Vault with name <yt_token_vault> ('yt-token' as default) and yt token as data.

By default task use arc for building binary and ci token as arc token. You can configure custom arc options by adding:

```yaml
use_arc: true
arc_oauth_token: <secret name>
```
Parameter 'arc_oauth_token' value must satisfy 'sec@ver#key' template, where
```
sec - seret name
ver - secret version (optional)
key - key in secret for token (optional, default value - 'arc-token')
```

If you want to change platform, add parameter:
```yaml
target_platform: <your platform>
```

If parameter 'integrational_check' set to 'true', DEPLOY_BINARY_TASK run subtasks using POST requests to API method
https://sandbox.yandex-team.ru/media/swagger-ui/index.html#/task/task_list_post


# Template

```yaml
service: <Your service name>
title: <Service title>
ci:
  secret: <ci-secret>
  runtime:
    sandbox-owner: <owner>
  releases:
    release-binary-tasks:
      title: <Release title>
      flow: release-binary-tasks
      stages:
        build_binary_tasks:
          title: <Build title>
        release_testing:
          title: <Testing>
        release_prestable:
          title: <Prestable>
        release_stable:
          title: <Stable>
      auto: true  # Start release process after commit

  flows:
    release-binary-tasks:
      title: <Flow title>
      jobs:
        build:
          title: <Build title>
          stage: build_binary_tasks
          task: common/sandbox/deploy_binary_task
          input:  # DEPLOY_BINARY_TASK input parameteres
            attrs:  # attributes of created resource with binary
              task_type: <task_type>  # set to use creted binary in LastBinaryTaskRelease
              sync_upload_to_mds: true  # upload resource to mds
            integrational_check: true  # remove if you don't need run task from created binary in dry mode
            integrational_check_payload: [{"type": "<task type in binary>"}] # remove if you don't need run task from created binary in dry mode. If it need, fill POST json requests for integrational tasks.
        release_testing:  # release binary to testing (optional)
          title: <test release binary>
          stage: release_testing
          needs: build  # Need only build stage, but you can depend on other stages
          manual: true  # Manual release task
          task: common/releases/release_to_sandbox
          input:
            config:
              sandbox_resource_type: SANDBOX_TASKS_BINARY
              common_release_data:
                release_stage: testing
        release_prestable:  # release binary to prestable (optional)
          title: <prestable release binary>
          stage: release_prestable
          needs: build  # Need only build stage, but you can depend on other stages
          task: common/releases/release_to_sandbox
          manual: true  # Manual release task
          input:
            config:
              sandbox_resource_type: SANDBOX_TASKS_BINARY
              common_release_data:
                release_stage: prestable
        release_stable:  # release binary to stable (optional)
          title: <stable release binary>
          stage: release_stable
          needs: build  # Need only build stage, but you can depend on other stages
          task: common/releases/release_to_sandbox
          manual: true  # Manual release task
          input:
            config:
              sandbox_resource_type: SANDBOX_TASKS_BINARY
              common_release_data:
                release_stage: stable
```

# Template explanation

There is one flow 'build' with 4 stages:

```
build - required stage. In this stage DEPLOY_BINARY_TASK creates SANDBOX_TASKS_BINARY resource.
release_testing - optional stage for releasing DEPLOY_BINARY_TASK from previous stage to TESTING.
  needs - this stage depends on 'build' stage.
  manual - you must start this stage by hand. Turn it to false for automate releasing to TESTING.
release_prestable - optional stage for releasing DEPLOY_BINARY_TASK from 'build' stage to PRESTABLE.
  needs - this stage depends on 'build' stage. You can make this stage to be depend on 'release_testing' stage.
  manual - you must start this stage by hand. Turn it to false for automate releasing to PRESTABLE.
release_stable - optional stage for releasing DEPLOY_BINARY_TASK from 'build' stage to STABLE.
  needs - this stage depends on 'build' stage. You can make this stage to be depend on 'release_testing' or 'release_prestable' stage.
  manual - you must start this stage by hand. Turn it to false for automate releasing to STABLE.
```

Put this template to a.yaml in building directory. Add 'target' input parameter if you put a.yaml not to building directory.

Fill your values instead of '<>' placeholders.

You can remove optional stages if you don't need them.
For example, if you don't need 'prestable' release, remove section 'release_prestable' from releases and flow.

You can insert your jobs between release jobs for run tests.

# Template of precommit build

```yaml
service: <Your service name>
title: <Service title>
ci:
  secret: <ci-secret>
  runtime:
    sandbox-owner: <owner>

  triggers:
    - on: pr
      flow: release-binary-tasks-for-testing

  flows:
    release-binary-tasks-for-testing:
      title: <Flow title>
      jobs:
        build:
          title: <Build title>
          stage: build_binary_tasks
          task: common/sandbox/deploy_binary_task
          input:  # DEPLOY_BINARY_TASK input parameteres
            attrs:  # attributes of created resource with binary
              task_type: <task_type>  # set to use creted binary in LastBinaryTaskRelease
              sync_upload_to_mds: true  # upload resource to mds
            integrational_check: true  # remove if you don't need run task from created binary in dry mode
            integrational_check_payload: [{"type": "<task type in binary>"}] # remove if you don't need run task from created binary in dry mode. If it need, fill POST json requests for integrational tasks.
```

It needs only to test binary before commit by hand. It must not to add release to this template.
