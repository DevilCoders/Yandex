# Configuration
If you use production sandbox you have to pass `OAUTH_TOKEN`  via `-T` option or via `SANDBOX_TOKEN` environment variable.
You can get it from [here](https://sandbox.yandex-team.ru/oauth)

# Usage Guide

## Run task
### Run sample task and wait it to finish
```
./bin/sandboxctl create -N RUN_SCRIPT -c cmdline='echo "Hello World"; lscpu > lscpu.txt' --wait
Starting task RUN_SCRIPT https://sandbox.yandex-team.ru/task/313745048/view
[58.55 s] SUCCESS
---
meta:
  id: 313745048
  info:
  - '2018-10-10 15:29:03 Process started. output[https://proxy.sandbox.yandex-team.ru/703591639/script_run.out.txt] '
  status: SUCCESS
  type: RUN_SCRIPT
  url: https://sandbox.yandex-team.ru/task/313745048/view
resources:
- description: Task logs
  proxy: https://proxy.sandbox.yandex-team.ru/703591639
  skynet_id: null
  url: http://sandbox.yandex-team.ru/api/v1.0/task/313745048/resources/703591639
```
### Task parameters
Most tasks require input parameters. For example RUN_SCRIPT task expect at least `cmdline` to be passed.
There are three ways to pass task parameters
#### Basic key=val parameter
Via command line option `-c KEY=VAL`
```
./bin/sandboxctl create -N RUN_SCRIPT -c cmdline='echo "Hello World"; lscpu > lscpu.txt' --wait
```
#### JSONstring
Via command line option `-C JSONSTRING`
```
bin/sandboxctl create  -N RUN_SCRIPT \
       -c cmdline='dmesg > dmesg.txt; lscpu > lscpu.txt' \
       -C '{"save_as_resource" : {"dmesg.txt":"OTHER_RESOURCE", "lscpu.txt": "OTHER_RESOURCE"}}' \
       --wait
```
#### Read task_parameters from manifest file (yml/json)
Just like `kubectl` does it.
Example:
```
./bin/sandboxctl create -i examples/01-run_script.yaml --wait
```
Obviously manifest file is the best choice for complex tasks with complex parameters. See [./examples directory](../examples) for more examples.
You can easily generate manifest from scratch via comandline 

```
## Save task config to manifest file, but not start it
./bin/sandboxctl create -N RUN_SCRIPT -c cmdline='echo "Hello World"; lscpu > lscpu.txt' -o script_task.yaml --dry-run
# Run command from manifest and wait it to finish 
 ./bin/sandboxctl create -i script_task.yaml -W
```
#### Customize manifest parameters
One may want to customize manifest parameters
Example:
```
# Run YA_MAKE task over local working (this x2-3 times faster than PR-engine crap)
patch_url=$(yarc diff | ya patse) && sandboxctl create -W -i infra/diskmanager/ci/kvmtest/yamake-sbtask.yml -c arcadia_patch=$patch_url
```
### task schedule options
One can use --require option, or 'requirement' in manifest see [examples/06-exec_script-lkvm.yaml](../examples/06-exec_script-lkvm.yaml)
```
# Execute task on given host
./bin/sandboxctl create -W --require host=sandbox598 -N EXEC_SCRIPT -c command='uname -a'                                                   
2019-03-19 15:59:34,233 INFO main:350 Starting task EXEC_SCRIPT https://sandbox.yandex-team.ru/task/397675136/view
2019-03-19 15:59:34,233 INFO main:350 Starting task EXEC_SCRIPT https://sandbox.yandex-team.ru/task/397675136/view
INFO:sandboxctl:Starting task EXEC_SCRIPT https://sandbox.yandex-team.ru/task/397675136/view
[53.91 s] SUCCESS 
# Execute task on host with PORTO and 32cpu cores
./bin/sandboxctl create -W -N EXEC_SCRIPT  --require cores=32 --require client_tags=PORTO -c command='portoctl'
```

### Run SANDBOX_TASK_BINARY
Sandbox binary tasks it a very cool feature [wiki](https://wiki.yandex-team.ru/sandbox/tasks/build), in fact once it will be implemented propery
it will makes this utility redundant. But at the moment task_bin has limited usability.
One can compile sandbox's task as binary for example like follows:
```
cd $ARC_ROOT
# Compile task-set as binary
ya make -t sandbox/projects/common/infra/bin/
sandbox/projects/common/infra/bin/sandbox-common-infra-tasks --help
usage: sandbox-common-infra-tasks [-h] <command> ...

Tasks binary.

optional arguments:
  -h, --help  show this help message and exit

subcommands:
  <command>   <description>
    run       Subcommand to run task.
    upload    Binary uploading subcommand.
    content   Binary content analysis.
    ipython   Run ipython using built tasks code.
```
That's it, no more dull debugging in local-sandbox, just compile and run it (under the carpet it will upload itself to prod-sandbox server and execute)
Once we have binary available you can execute it as normal tasks (just add `-b` option)
```
cd $ARC_ROOT
./tools/sandboxctl/bin/sandboxctl create\
      -W \
      -i script_task.yaml
      -b ./sandbox/projects/common/infra/bin/sandbox-common-infra-tasks
Use sandbox_url: https://sandbox.yandex-team.ru                
2018-10-22 17:10:08     INFO    Use already uploaded resource: https://sandbox.yandex-team.ru/resource/716361049
Use SANDBOX_TASK_BINARY resource_id:716361049                                                                                                                                                                       
Starting task EXEC_SCRIPT_LXC https://sandbox.yandex-team.ru/task/320113439/view
.....
```

## Get task info
```
### Create task w/o waiting
TASK_ID=$(./bin/sandboxctl create -N TEST_TASK -q)
### Dump task info 
./bin/sandboxctl get_task  $TASK_ID
### Dump full task info
./bin/sandboxctl get_task  $TASK_ID --full
### Wait exiting task to finish 
./bin/sandboxctl get_task  $TASK_ID --wait
```

## Clone task example
```
# Save existing task manifest
./bin/sandboxctl get_task $TASK_ID --manifest > task_to_clone.yml
# Fix parameters if required
$EDITOR task_to_clone.yml
# Submit task 
./bin/sandboxctl create -W -i task_to_clone.yml
```


## Interactive task debug
```
TASK_ID=$(./bin/sandboxctl create -N RUN_SCRIPT -c cmdline='sleep 300' -q)
./bin/sandboxctl suspend -W $TASK_ID
## web-console will be  showed, so one can access it
# Resume and wait task
./bin/sandboxctl resume -W $TASK_ID
./bin/sandboxctl get_task -W $TASK_ID
```

## Work with resources
### List resources
Lookup  latest released resource with custom attrs
```
$ sandboxctl list_resource  --limit 1 --type ARCADIA_PROJECT_TGZ --attr arcadia_path=infra/environments/qavm-bionic/vm-image/rootfs.img --attr released=stable
```
Lookup resource declarated in ya.make.autoupdate [see](https://wiki.yandex-team.ru/users/nalpp/autoupdated-resources)
```
$ sandboxctl list_resource --limit 1 -q  --jq infra/environments/qavm-bionic/release/vm-image/ya.make.autoupdate 
1066637576
```
### Lookup and fetch resource
```
id=$(sandboxctl list_resource --limit 1 -q  --jq infra/environments/qavm-bionic/release/vm-image/ya.make.autoupdate)
$ sandboxctl get_resource -q $id
/home/dmtrmonakhov/.ya/sandbox-storage/1066637576/content/rootfs.img.tar.gz
```
### Fetch resources
```
# Fetch sandbox resource by ID
$ bin/sandboxctl get_resource  -q  860767873
/home/dmtrmonakhov/.ya/sandbox-storage/860767873/content/qemu-lite-dev-container.tar.gz
# EXAMPLE: Fetch docker image as sandbox_resource and load it to docker
$ docker load -i $(bin/sandboxctl get_resource  -q  860767873)
7faa90c9b8f7: Loading layer [==================================================>]  24.81MB/24.81MB
b1686bd1b8ce: Loading layer [==================================================>]    390MB/390MB
Loaded image: qemu-lite-dev-container:latest
```

## Run ya.make in sandbox
sandboxctl has special shortcat **ya-make** for building arcadia artifacts in sandbox.
This short cat starts [YA_MAKE_TGZ](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/common/infra/ya_make_tgz/__init__.py)
task, which is improved version of YA_MAKE. This gives you look and feel similar to ``` ya make --dist```

Please use this method for saring your binaries instead of local hack ```ya make XX && ya upload XX``` because artifacts produced by YA_MAKE_TGZ
has attributes of origin (revision, date, etc)

**Examples**:
Run command below from your arcadia root to build sandboxctl binary and run all-test, later result artifact may be released
```
sandboxctl ya-make -W -A tools/sandboxctl/bin/sandboxctl
```

Build artifact with local modifications against trunk
```
sandboxctl ya-make -W tools/sandboxctl/bin/sandboxctl  --patch=$(arc diff trunk.. | ya paste)
```

```
sandboxctl ya-make -A -W \
	   infra/environments/vanilla-bionic-vm/layer/layer.tar.gz \
	   infra/environments/qavm-bionic/layer/layer.tar.gz \
	   infra/environments/qavm-bionic/vm-image/rootfs.img \
	   \
	   infra/environments/vanilla-xenial-vm/layer/layer.tar.gz \
	   infra/environments/qavm-xenial/layer/layer.tar.gz \
	   infra/environments/qavm-xenial/vm-image/rootfs.img \
```

## Build from arc_vcs branch, 
**Examples**:

Just add ```-u arcadia-arc://#<rev-spec>```, [more info about arc support in sandbox](https://clubs.at.yandex-team.ru/arcadia/20332)
Run ya-make over my current HEAD
```
#Push changes to remote server first, so sandbox can access it
arc push users/$USER/my-dev-branch
sandboxctl ya-make -A -W -u HEAD tools/sandboxctl/bin/sandboxctl
```
Build from given branch
```
sandboxctl ya-make -A -W -u users/dmtrmonakhov/sandboxctl-dev  tools/sandboxctl/bin/sandboxctl
```
Git-like revspec also supported, build from two commits ahead trunk
```
sandboxctl ya-make -A -W -u trunk^^  tools/sandboxctl/bin/sandboxctl
```
Build several artifacts in one task, each artifact will be released as separate resource
